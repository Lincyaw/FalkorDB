/*
* Copyright 2018-2022 Redis Labs Ltd. and Contributors
*
* This file is available under the Redis Labs Source Available License Agreement
*/

#include "encode_context.h"
#include "../RG.h"
#include "../util/rmalloc.h"
#include "../util/rax_extensions.h"
#include "../configuration/config.h"

GraphEncodeContext *GraphEncodeContext_New() {
	GraphEncodeContext *ctx = rm_calloc(1, sizeof(GraphEncodeContext));
	ctx->meta_keys = raxNew();
	GraphEncodeContext_Reset(ctx);
	return ctx;
}

static void _GraphEncodeContext_ResetHeader(GraphEncodeContext *ctx) {
	ASSERT(ctx != NULL);

	GraphEncodeHeader *header = &(ctx->header);

	header->key_count = 0;
	header->node_count = 0;
	header->edge_count = 0;
	header->graph_name = NULL;
	header->label_matrix_count = 0;
	header->relationship_matrix_count = 0;
}

void GraphEncodeContext_Reset(GraphEncodeContext *ctx) {
	ASSERT(ctx != NULL);

	_GraphEncodeContext_ResetHeader(ctx);

	ctx->offset = 0;
	ctx->keys_processed = 0;
	ctx->state = ENCODE_STATE_INIT;
	ctx->current_relation_matrix_id = 0;

	Config_Option_get(Config_VKEY_MAX_ENTITY_COUNT, &ctx->vkey_entity_count);

	// Avoid leaks in case or reset during encodeing.
	if(ctx->datablock_iterator != NULL) {
		DataBlockIterator_Free(ctx->datablock_iterator);
		ctx->datablock_iterator = NULL;
	}

	// Avoid leaks in case or reset during encodeing.
	RG_MatrixTupleIter_detach(&ctx->matrix_tuple_iterator);
}

void GraphEncodeContext_InitHeader(GraphEncodeContext *ctx, const char *graph_name, Graph *g) {
	ASSERT(g   != NULL);
	ASSERT(ctx != NULL);

	int r_count = Graph_RelationTypeCount(g);
	GraphEncodeHeader *header = &(ctx->header);

	header->graph_name                 =  graph_name;
	header->node_count                 =  Graph_NodeCount(g);
	header->edge_count                 =  Graph_EdgeCount(g);
	header->relationship_matrix_count  =  r_count;
	header->label_matrix_count         =  Graph_LabelTypeCount(g);
	header->key_count                  =  GraphEncodeContext_GetKeyCount(ctx);
}

EncodeState GraphEncodeContext_GetEncodeState(const GraphEncodeContext *ctx) {
	ASSERT(ctx);
	return ctx->state;
}

void GraphEncodeContext_SetEncodeState(GraphEncodeContext *ctx, EncodeState state) {
	ASSERT(ctx);
	ctx->state = state;
}

uint64_t GraphEncodeContext_GetKeyCount(const GraphEncodeContext *ctx) {
	ASSERT(ctx);
	// The `meta_keys` rax contains only the meta keys names. Add one for the graph context key.
	return raxSize(ctx->meta_keys) + 1;
}

void GraphEncodeContext_AddMetaKey(GraphEncodeContext *ctx, const char *key) {
	ASSERT(ctx);
	raxInsert(ctx->meta_keys, (unsigned char *)key, strlen(key), NULL, NULL);
}

unsigned char **GraphEncodeContext_GetMetaKeys(const GraphEncodeContext *ctx) {
	ASSERT(ctx);
	return raxKeys(ctx->meta_keys);
}

void GraphEncodeContext_ClearMetaKeys(GraphEncodeContext *ctx) {
	ASSERT(ctx);
	raxFree(ctx->meta_keys);
	ctx->meta_keys = raxNew();
}

uint64_t GraphEncodeContext_GetProcessedKeyCount(const GraphEncodeContext *ctx) {
	ASSERT(ctx);
	return ctx->keys_processed;
}

uint64_t GraphEncodeContext_GetProcessedEntitiesOffset(const GraphEncodeContext *ctx) {
	ASSERT(ctx);
	return ctx->offset;
}

void GraphEncodeContext_SetProcessedEntitiesOffset(GraphEncodeContext *ctx,
												   uint64_t offset) {
	ASSERT(ctx);
	ctx->offset = offset;
}

DataBlockIterator *GraphEncodeContext_GetDatablockIterator(const GraphEncodeContext *ctx) {
	ASSERT(ctx);
	return ctx->datablock_iterator;
}

void GraphEncodeContext_SetDatablockIterator(GraphEncodeContext *ctx,
											 DataBlockIterator *iter) {
	ASSERT(ctx);
	ctx->datablock_iterator = iter;
}

uint GraphEncodeContext_GetCurrentRelationID(const GraphEncodeContext *ctx) {
	ASSERT(ctx);
	return ctx->current_relation_matrix_id;
}

void GraphEncodeContext_SetCurrentRelationID(GraphEncodeContext *ctx,
											 uint current_relation_matrix_id) {
	ASSERT(ctx);
	ctx->current_relation_matrix_id = current_relation_matrix_id;
}

RG_MatrixTupleIter *GraphEncodeContext_GetMatrixTupleIterator(
	GraphEncodeContext *ctx) {
	ASSERT(ctx);
	return &ctx->matrix_tuple_iterator;
}

bool GraphEncodeContext_Finished(const GraphEncodeContext *ctx) {
	ASSERT(ctx);
	return ctx->keys_processed == GraphEncodeContext_GetKeyCount(ctx);
}

void GraphEncodeContext_IncreaseProcessedKeyCount(GraphEncodeContext *ctx) {
	ASSERT(ctx);
	ASSERT(ctx->keys_processed < GraphEncodeContext_GetKeyCount(ctx));
	ctx->keys_processed++;
}

void GraphEncodeContext_Free(GraphEncodeContext *ctx) {
	if(ctx) {
		raxFree(ctx->meta_keys);
		rm_free(ctx);
	}
}

