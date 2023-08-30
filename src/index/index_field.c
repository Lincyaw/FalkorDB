/*
 * Copyright FalkorDB Ltd. 2023 - present
 * Licensed under the Server Side Public License v1 (SSPLv1).
 */

#include "RG.h"
#include "index_field.h"
#include "../util/rmalloc.h"

//------------------------------------------------------------------------------
// index field creation
//------------------------------------------------------------------------------

static void _ResetFulltextOptions
(
	IndexField *f
) {
	ASSERT(f != NULL);

	rm_free(f->options.phonetic);

	f->options.weight   = INDEX_FIELD_DEFAULT_WEIGHT;
	f->options.nostem   = INDEX_FIELD_DEFAULT_NOSTEM;
	f->options.phonetic = rm_strdup(INDEX_FIELD_DEFAULT_PHONETIC);
}

static void _ResetVectorOptions
(
	IndexField *f
) {
	ASSERT(f != NULL);

	f->options.dimension = 0;
}

// initialize index field
void IndexField_Init
(
	IndexField *field,   // field to initialize
	const char *name,    // field name
	Attribute_ID id,     // attribute ID
	IndexFieldType type  // field type
) {
	ASSERT(name     != NULL);
	ASSERT(field    != NULL);

	// clear field
	memset(field, 0, sizeof(IndexField));

	field->id   = id;
	field->name = rm_strdup(name);
	field->type = type;

	// set default options
	field->options.weight    = INDEX_FIELD_DEFAULT_WEIGHT;
	field->options.nostem    = INDEX_FIELD_DEFAULT_NOSTEM;
	field->options.phonetic  = rm_strdup(INDEX_FIELD_DEFAULT_PHONETIC);
	field->options.dimension = 0;

	if(type & INDEX_FLD_FULLTEXT) {
		field->fulltext_name = field->name;
	}
	if(type & INDEX_FLD_RANGE) {
		asprintf(&field->range_name, "range:%s", name);
	}
	if(type & INDEX_FLD_VECTOR) {
		asprintf(&field->vector_name, "vector:%s", name);
	}
}

// set index field options
// note not all options are applicable to all field types
void IndexField_SetOptions
(
	IndexField *field,  // field to update
	double weight,      // field's weight
	bool nostem,        // field's stemming
	char *phonetic,     // field's phonetic
	uint32_t dimension  // field's vector dimension
) {
	ASSERT(field != NULL);
	ASSERT(phonetic != NULL);

	// default options
	ASSERT(field->options.dimension == 0);
	ASSERT(field->options.weight    == INDEX_FIELD_DEFAULT_WEIGHT);
	ASSERT(field->options.nostem    == INDEX_FIELD_DEFAULT_NOSTEM);
	ASSERT(strcmp(field->options.phonetic, INDEX_FIELD_DEFAULT_PHONETIC) == 0);

	// set options
	field->options.weight    = weight;
	field->options.nostem    = nostem;
	field->options.dimension = dimension;

	if(phonetic != NULL) {
		rm_free(field->options.phonetic);
		field->options.phonetic	= rm_malloc(strlen(phonetic)+1);
		strcpy(field->options.phonetic, phonetic);
	}
}

// create a new range index field
void IndexField_NewRangeField
(
	IndexField *field,   // field to initialize
	const char *name,    // field name
	Attribute_ID id      // field id
) {
	IndexFieldType t = INDEX_FLD_RANGE;
	IndexField_Init(field, name, id, t);
}

// create a new full text index field
void IndexField_NewFullTextField
(
	IndexField *field,   // field to initialize
	const char *name,    // field name
	Attribute_ID id      // field id
) {
	IndexFieldType t = INDEX_FLD_FULLTEXT;
	IndexField_Init(field, name, id, t);
}

// create a new vector index field
void IndexField_NewVectorField
(
	IndexField *field,   // field to initialize
	const char *name,    // field name
	Attribute_ID id,     // field id
	uint32_t dimension   // vector dimension
) {
	IndexField_Init(field, name, id, INDEX_FLD_VECTOR);
	IndexField_SetDimension(field, dimension);
}

// clone index field
void IndexField_Clone
(
	const IndexField *src,  // field to clone
	IndexField *dest        // cloned field
) {
	ASSERT(src  != NULL);
	ASSERT(dest != NULL);

	memcpy(dest, src, sizeof(IndexField));

	dest->name = rm_strdup(src->name);

	if(src->options.phonetic != NULL) {
		dest->options.phonetic = rm_strdup(src->options.phonetic);
	}

	//--------------------------------------------------------------------------
	// clone type specific field names
	//--------------------------------------------------------------------------

	if(src->type & INDEX_FLD_FULLTEXT) {
		dest->fulltext_name = dest->name;
	}
	if(src->type & INDEX_FLD_RANGE) {
		dest->range_name = rm_strdup(src->range_name);
	}
	if(src->type & INDEX_FLD_VECTOR) {
		dest->vector_name = rm_strdup(src->vector_name);
	}
}

inline IndexFieldType IndexField_GetType
(
	const IndexField *f  // field to get type
) {
	ASSERT(f != NULL);

	return f->type;
}

const char *IndexField_GetName
(
	const IndexField *f  // field to get name
) {
	ASSERT(f != NULL);	
	
	return f->name;
}

// remove type from field
void IndexField_RemoveType
(
	IndexField *f,    // field to update
	IndexFieldType t  // type to remove
) {
	ASSERT(f != NULL);
	ASSERT(t & (INDEX_FLD_RANGE | INDEX_FLD_FULLTEXT | INDEX_FLD_VECTOR));
	ASSERT(f->type & t);

	// remove RANGE type
	if(t & INDEX_FLD_RANGE) {
		rm_free(f->range_name);
		f->range_name = NULL;
	}

	// remove FULLTEXT type
	if(t & INDEX_FLD_FULLTEXT) {
		f->fulltext_name = NULL;
		_ResetFulltextOptions(f);
	}

	// remove VECTOR type
	if(t & INDEX_FLD_VECTOR) {
		rm_free(f->vector_name);
		f->vector_name = NULL;
		_ResetVectorOptions(f);
	}

	f->type &= ~t;
}

//------------------------------------------------------------------------------
// index field options
//------------------------------------------------------------------------------

// set index field weight
void IndexField_SetWeight
(
	IndexField *field,  // field to update
	double weight       // new weight
) {
	ASSERT(field != NULL);
	field->options.weight = weight;
}

// set index field stemming
void IndexField_SetStemming
(
	IndexField *field,  // field to update
	bool nostem         // enable/disable stemming
) {
	ASSERT(field != NULL);
	field->options.nostem = nostem;
}

// set index field phonetic
void IndexField_SetPhonetic
(
	IndexField *field,    // field to update
	const char *phonetic  // phonetic
) {
	ASSERT(field    != NULL);
	ASSERT(phonetic != NULL);

	field->options.phonetic = rm_strdup(phonetic);
}

// set index field vector dimension
void IndexField_SetDimension
(
	IndexField *field,  // field to update
	uint32_t dimension  // vector dimension
) {
	ASSERT(field != NULL);
	field->options.dimension = dimension;
}

// free index field
void IndexField_Free
(
	IndexField *field
) {
	ASSERT(field != NULL);

	rm_free(field->name);
	rm_free(field->options.phonetic);

	// free type specific field names
	if(field->range_name  != NULL) rm_free(field->range_name);
	if(field->vector_name != NULL) rm_free(field->vector_name);
}

