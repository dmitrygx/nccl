/*************************************************************************
 * Copyright (c) 2004, 2005 Topspin Communications.  All rights reserved.
 * Copyright (c) 2004, 2011-2012 Intel Corporation.  All rights reserved.
 * Copyright (c) 2005, 2006, 2007 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2005 PathScale, Inc.  All rights reserved.
 *
 * Copyright (c) 2015-2022, NVIDIA CORPORATION. All rights reserved.
 *
 * See LICENSE.txt for license information
 ************************************************************************/

#ifndef NCCL_IBVWRAP_H_
#define NCCL_IBVWRAP_H_

#include <cstdint>
#ifndef container_of
#define nccl_offsetof(_type, _member) \
    ((unsigned long)&( ((_type*)0)->_member ))
#define container_of(_ptr, _type, _member) \
    ( (_type*)( (char*)(void*)(_ptr) - nccl_offsetof(_type, _member) )  )
#endif

#ifdef NCCL_BUILD_RDMA_CORE
#include <infiniband/verbs.h>
#else
#include "ibvcore.h"
#endif

#ifdef NCCL_BUILD_MLX5DV
#include <infiniband/mlx5dv.h>
#else

#endif

#include "core.h"
#include <sys/types.h>
#include <unistd.h>

typedef enum ibv_return_enum
{
    IBV_SUCCESS = 0,                   //!< The operation was successful
} ibv_return_t;

ncclResult_t wrap_ibv_symbols(void);
ncclResult_t wrap_mlx5_symbols(void);
/* NCCL wrappers of IB verbs functions */
ncclResult_t wrap_ibv_fork_init(void);
ncclResult_t wrap_ibv_get_device_list(struct ibv_device ***ret, int *num_devices);
ncclResult_t wrap_ibv_free_device_list(struct ibv_device **list);
const char *wrap_ibv_get_device_name(struct ibv_device *device);
ncclResult_t wrap_ibv_open_device(struct ibv_context **ret, struct ibv_device *device);
ncclResult_t wrap_ibv_close_device(struct ibv_context *context);
ncclResult_t wrap_ibv_get_async_event(struct ibv_context *context, struct ibv_async_event *event);
ncclResult_t wrap_ibv_ack_async_event(struct ibv_async_event *event);
ncclResult_t wrap_ibv_query_device(struct ibv_context *context, struct ibv_device_attr *device_attr);
ncclResult_t wrap_ibv_query_device_ex(struct ibv_context *context, const struct ibv_query_device_ex_input *input, struct ibv_device_attr_ex *attr);
ncclResult_t wrap_ibv_query_port(struct ibv_context *context, uint8_t port_num, struct ibv_port_attr *port_attr);
ncclResult_t wrap_ibv_query_gid(struct ibv_context *context, uint8_t port_num, int index, union ibv_gid *gid);
ncclResult_t wrap_ibv_query_qp(struct ibv_qp *qp, struct ibv_qp_attr *attr, int attr_mask, struct ibv_qp_init_attr *init_attr);
ncclResult_t wrap_ibv_alloc_pd(struct ibv_pd **ret, struct ibv_context *context);
ncclResult_t wrap_ibv_dealloc_pd(struct ibv_pd *pd);
ncclResult_t wrap_ibv_reg_mr(struct ibv_mr **ret, struct ibv_pd *pd, void *addr, size_t length, int access);
struct ibv_mr * wrap_direct_ibv_reg_mr(struct ibv_pd *pd, void *addr, size_t length, int access);
ncclResult_t wrap_ibv_reg_mr_iova2(struct ibv_mr **ret, struct ibv_pd *pd, void *addr, size_t length, uint64_t iova, int access);
/* DMA-BUF support */
ncclResult_t wrap_ibv_reg_dmabuf_mr(struct ibv_mr **ret, struct ibv_pd *pd, uint64_t offset, size_t length, uint64_t iova, int fd, int access);
struct ibv_mr * wrap_direct_ibv_reg_dmabuf_mr(struct ibv_pd *pd, uint64_t offset, size_t length, uint64_t iova, int fd, int access);
ncclResult_t wrap_ibv_dereg_mr(struct ibv_mr *mr);
ncclResult_t wrap_ibv_create_comp_channel(struct ibv_comp_channel **ret, struct ibv_context *context);
ncclResult_t wrap_ibv_destroy_comp_channel(struct ibv_comp_channel *channel);
ncclResult_t wrap_ibv_create_cq(struct ibv_cq **ret, struct ibv_context *context, int cqe, void *cq_context, struct ibv_comp_channel *channel, int comp_vector);
ncclResult_t wrap_ibv_create_cq_ex(struct ibv_cq_ex **ret, struct ibv_context *context, struct ibv_cq_init_attr_ex *init_attr);
ncclResult_t wrap_ibv_destroy_cq(struct ibv_cq *cq);
static inline ncclResult_t wrap_ibv_poll_cq(struct ibv_cq *cq, int num_entries, struct ibv_wc *wc, int* num_done) {
  int done = cq->context->ops.poll_cq(cq, num_entries, wc); /*returns the number of wcs or 0 on success, a negative number otherwise*/
  if (done < 0) {
    WARN("Call to ibv_poll_cq() returned %d", done);
    return ncclSystemError;
  }
  *num_done = done;
  return ncclSuccess;
}
static inline ibv_cq* wrap_ibv_cq_ex_to_cq(struct ibv_cq_ex *cq_ex) {
  return (struct ibv_cq*)cq_ex;
}
static inline ncclResult_t wrap_ibv_start_poll(struct ibv_cq_ex *cq, struct ibv_poll_cq_attr *attr, int *done)
{
  int ret = cq->start_poll(cq, attr);
  if (ret && ret != ENOENT) {
    WARN("Call to ibv_start_poll() returned %d", ret);
    *done = 0;
    return ncclSystemError;
  }
  *done = (ret != ENOENT);
	return ncclSuccess;
}

static inline ncclResult_t wrap_ibv_next_poll(struct ibv_cq_ex *cq, int *done)
{
  int ret = cq->next_poll(cq);
  if (ret && ret != ENOENT) {
    WARN("Call to ibv_next_poll() returned %d", ret);
    *done = 0;
    return ncclSystemError;
  }
  *done = (ret != ENOENT);
	return ncclSuccess;
}

static inline void wrap_ibv_end_poll(struct ibv_cq_ex *cq)
{
	cq->end_poll(cq);
}
static inline uint64_t wrap_ibv_wc_read_completion_ts(struct ibv_cq_ex *cq)
{
	return cq->read_completion_ts(cq);
}
static inline enum ibv_wc_opcode wrap_ibv_wc_read_opcode(struct ibv_cq_ex *cq)
{
	return cq->read_opcode(cq);
}
static inline uint32_t wrap_ibv_wc_read_vendor_err(struct ibv_cq_ex *cq)
{
	return cq->read_vendor_err(cq);
}
static inline uint32_t wrap_ibv_wc_read_byte_len(struct ibv_cq_ex *cq)
{
	return cq->read_byte_len(cq);
}
static inline __be32 wrap_ibv_wc_read_imm_data(struct ibv_cq_ex *cq)
{
	return cq->read_imm_data(cq);
}
ncclResult_t wrap_ibv_create_qp(struct ibv_qp **ret, struct ibv_pd *pd, struct ibv_qp_init_attr *qp_init_attr);
ncclResult_t wrap_ibv_modify_qp(struct ibv_qp *qp, struct ibv_qp_attr *attr, int attr_mask);
ncclResult_t wrap_ibv_destroy_qp(struct ibv_qp *qp);

static inline ncclResult_t wrap_ibv_post_send(struct ibv_qp *qp, struct ibv_send_wr *wr, struct ibv_send_wr **bad_wr) {
  int ret = qp->context->ops.post_send(qp, wr, bad_wr); /*returns 0 on success, or the value of errno on failure (which indicates the failure reason)*/
  if (ret != IBV_SUCCESS) {
    WARN("ibv_post_send() failed with error %s, Bad WR %p, First WR %p", strerror(ret), wr, *bad_wr);
    return ncclSystemError;
  }
  return ncclSuccess;
}

static inline ncclResult_t wrap_ibv_post_recv(struct ibv_qp *qp, struct ibv_recv_wr *wr, struct ibv_recv_wr **bad_wr) {
  int ret = qp->context->ops.post_recv(qp, wr, bad_wr); /*returns 0 on success, or the value of errno on failure (which indicates the failure reason)*/
  if (ret != IBV_SUCCESS) {
    WARN("ibv_post_recv() failed with error %s", strerror(ret));
    return ncclSystemError;
  }
  return ncclSuccess;
}

ncclResult_t wrap_ibv_event_type_str(char **ret, enum ibv_event_type event);

ncclResult_t wrap_mlx5dv_get_clock_info(struct ibv_context *ctx_in, struct mlx5dv_clock_info *clock_info);
uint64_t wrap_mlx5dv_ts_to_ns(struct mlx5dv_clock_info *clock_info, uint64_t device_timestamp);

#endif //End include guard
