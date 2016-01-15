#include "rava.h"

void ravaL_object_init(rava_state_t* state, rava_object_t* self) {
  QUEUE_INIT(&self->rouse);
  QUEUE_INIT(&self->queue);

  self->state = state;
  self->data  = NULL;
  self->flags = 0;
  self->count = 0;
  self->buf.base = NULL;
  self->buf.len  = 0;
}

void ravaL_object_close_cb(uv_handle_t* handle) {
  rava_object_t* self = container_of(handle, rava_object_t, h);
  TRACE("object closed %p\n", self);
  self->flags |= RAVA_OCLOSED;
  self->state = NULL;
  ravaL_cond_signal(&self->rouse);
}

void ravaL_object_close(rava_object_t* self) {
  if (!ravaL_object_is_closing(self)) {
    TRACE("object closing %p, type: %i\n", self, self->h.handle.type);
    self->flags |= RAVA_OCLOSING;
    uv_close(&self->h.handle, ravaL_object_close_cb);
  }
}

