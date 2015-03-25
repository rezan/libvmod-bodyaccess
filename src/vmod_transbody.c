#include "vmod_core.h"

VCL_VOID
vmod_cache_req_body(VRT_CTX, double maxsize)
{
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(ctx->req, REQ_MAGIC);

	if (ctx->method != VCL_MET_RECV) {
		VSLb(ctx->vsl, SLT_VCL_Error,
		    "req.body can only be cached in vcl_recv{}");
			return;
	}
	VRB_Cache(ctx->req, maxsize);
}


VCL_VOID
vmod_hash_req_body(VRT_CTX)
{
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	struct vmod priv_top = { 0 };

	if (ctx->method != VCL_MET_HASH) {
		VSLb(ctx->vsl, SLT_VCL_Error,
		    "Hash_Req_Body can only be used in vcl_hash{}");
		return;
	}

	vmod_blob_req_body(ctx, priv_top);

	HSH_AddBytes(ctx->req, priv_top.priv,  priv_top.len);

}

VCL_INT
vmod_len_req_body(VRT_CTX)
{
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(ctx->req, REQ_MAGIC);

	if (ctx->req->req_body_status != REQ_BODY_CACHED) {
		VSLb(ctx->vsl, SLT_VCL_Error,
		   "Uncached req.body");

		return (-1);
	}

	return (ctx->req->req_bodybytes);
}


VCL_INT
vmod_rematch_req_body(VRT_CTX, struct vmod_priv *priv_call, VCL_STRING re)
{
	const char *error;
	int erroroffset;
	vre_t *t = NULL;
	int i;
	struct vmod priv_top = { 0 };

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	AN(re);

	if(priv_call->priv == NULL) {
		t =  VRE_compile(re, 0, &error, &erroroffset);
		priv_call->priv = t;
		priv_call->free = free;

		if(t == NULL) {
			VSLb(ctx->vsl, SLT_VCL_Error,
			    "Regular expression not valid");
			return (-1);
		}
	}
	cache_param->vre_limits.match = 10000;
	cache_param->vre_limits.match_recursion = 10000;
	priv_top = vmod_blob_req_body(ctx, priv_top);
	VSL(SLT_Debug, 0, "priv_top len %zd", priv_top.len);
	VSL(SLT_Debug, 0, "priv_top BLOB %s", (char*)priv_top.priv);
	i = VRE_exec(priv_call->priv, priv_top.priv, priv_top.len, 0, 0,
	    NULL, 0,&cache_param->vre_limits);

	if (i > 0)
		return (1);

	if (i == VRE_ERROR_NOMATCH )
		return (0);

	VSLb(ctx->vsl, SLT_VCL_Error, "Regexp matching returned %d", i);
		return (-1);

}
