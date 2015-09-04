/*
 * nghttp2 - HTTP/2 C Library
 *
 * Copyright (c) 2015 Tatsuhiro Tsujikawa
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "shrpx_mruby_module_request.h"

#include <mruby/variable.h>
#include <mruby/string.h>
#include <mruby/hash.h>
#include <mruby/array.h>

#include "shrpx_downstream.h"
#include "shrpx_upstream.h"
#include "shrpx_client_handler.h"
#include "shrpx_mruby.h"
#include "shrpx_mruby_module.h"
#include "util.h"
#include "http2.h"

namespace shrpx {

namespace mruby {

namespace {
mrb_value request_init(mrb_state *mrb, mrb_value self) { return self; }
} // namespace

namespace {
mrb_value request_get_http_version_major(mrb_state *mrb, mrb_value self) {
  auto data = static_cast<MRubyAssocData *>(mrb->ud);
  auto downstream = data->downstream;
  return mrb_fixnum_value(downstream->get_request_major());
}
} // namespace

namespace {
mrb_value request_get_http_version_minor(mrb_state *mrb, mrb_value self) {
  auto data = static_cast<MRubyAssocData *>(mrb->ud);
  auto downstream = data->downstream;
  return mrb_fixnum_value(downstream->get_request_minor());
}
} // namespace

namespace {
mrb_value request_get_method(mrb_state *mrb, mrb_value self) {
  auto data = static_cast<MRubyAssocData *>(mrb->ud);
  auto downstream = data->downstream;
  auto method = http2::to_method_string(downstream->get_request_method());

  return mrb_str_new_cstr(mrb, method);
}
} // namespace

namespace {
mrb_value request_set_method(mrb_state *mrb, mrb_value self) {
  auto data = static_cast<MRubyAssocData *>(mrb->ud);
  auto downstream = data->downstream;

  check_phase(mrb, data->phase, PHASE_REQUEST);

  const char *method;
  mrb_int n;
  mrb_get_args(mrb, "s", &method, &n);
  if (n == 0) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "method must not be empty string");
  }
  auto token =
      http2::lookup_method_token(reinterpret_cast<const uint8_t *>(method), n);
  if (token == -1) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "method not supported");
  }

  downstream->set_request_method(token);

  return self;
}
} // namespace

namespace {
mrb_value request_get_authority(mrb_state *mrb, mrb_value self) {
  auto data = static_cast<MRubyAssocData *>(mrb->ud);
  auto downstream = data->downstream;
  auto &authority = downstream->get_request_http2_authority();

  return mrb_str_new(mrb, authority.c_str(), authority.size());
}
} // namespace

namespace {
mrb_value request_set_authority(mrb_state *mrb, mrb_value self) {
  auto data = static_cast<MRubyAssocData *>(mrb->ud);
  auto downstream = data->downstream;

  check_phase(mrb, data->phase, PHASE_REQUEST);

  const char *authority;
  mrb_int n;
  mrb_get_args(mrb, "s", &authority, &n);
  if (n == 0) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "authority must not be empty string");
  }

  downstream->set_request_http2_authority(std::string(authority, n));

  return self;
}
} // namespace

namespace {
mrb_value request_get_scheme(mrb_state *mrb, mrb_value self) {
  auto data = static_cast<MRubyAssocData *>(mrb->ud);
  auto downstream = data->downstream;
  auto &scheme = downstream->get_request_http2_scheme();

  return mrb_str_new(mrb, scheme.c_str(), scheme.size());
}
} // namespace

namespace {
mrb_value request_set_scheme(mrb_state *mrb, mrb_value self) {
  auto data = static_cast<MRubyAssocData *>(mrb->ud);
  auto downstream = data->downstream;

  check_phase(mrb, data->phase, PHASE_REQUEST);

  const char *scheme;
  mrb_int n;
  mrb_get_args(mrb, "s", &scheme, &n);
  if (n == 0) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "scheme must not be empty string");
  }

  downstream->set_request_http2_scheme(std::string(scheme, n));

  return self;
}
} // namespace

namespace {
mrb_value request_get_path(mrb_state *mrb, mrb_value self) {
  auto data = static_cast<MRubyAssocData *>(mrb->ud);
  auto downstream = data->downstream;
  auto &path = downstream->get_request_path();

  return mrb_str_new(mrb, path.c_str(), path.size());
}
} // namespace

namespace {
mrb_value request_set_path(mrb_state *mrb, mrb_value self) {
  auto data = static_cast<MRubyAssocData *>(mrb->ud);
  auto downstream = data->downstream;

  check_phase(mrb, data->phase, PHASE_REQUEST);

  const char *path;
  mrb_int pathlen;
  mrb_get_args(mrb, "s", &path, &pathlen);

  downstream->set_request_path(std::string(path, pathlen));

  return self;
}
} // namespace

namespace {
mrb_value request_get_headers(mrb_state *mrb, mrb_value self) {
  auto data = static_cast<MRubyAssocData *>(mrb->ud);
  auto downstream = data->downstream;
  return create_headers_hash(mrb, downstream->get_request_headers());
}
} // namespace

namespace {
mrb_value request_mod_header(mrb_state *mrb, mrb_value self, bool repl) {
  auto data = static_cast<MRubyAssocData *>(mrb->ud);
  auto downstream = data->downstream;

  check_phase(mrb, data->phase, PHASE_REQUEST);

  mrb_value key, values;
  mrb_get_args(mrb, "oo", &key, &values);

  if (RSTRING_LEN(key) == 0) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "empty key is not allowed");
  }

  key = mrb_funcall(mrb, key, "downcase", 0);

  if (repl) {
    // making name empty will effectively delete header fields
    for (auto &hd : downstream->get_request_headers()) {
      if (util::streq(std::begin(hd.name), hd.name.size(), RSTRING_PTR(key),
                      RSTRING_LEN(key))) {
        hd.name = "";
      }
    }
  }

  if (mrb_obj_is_instance_of(mrb, values, mrb->array_class)) {
    auto n = mrb_ary_len(mrb, values);
    for (int i = 0; i < n; ++i) {
      auto value = mrb_ary_entry(values, i);
      downstream->add_request_header(
          std::string(RSTRING_PTR(key), RSTRING_LEN(key)),
          std::string(RSTRING_PTR(value), RSTRING_LEN(value)));
    }
  } else if (!mrb_nil_p(values)) {
    downstream->add_request_header(
        std::string(RSTRING_PTR(key), RSTRING_LEN(key)),
        std::string(RSTRING_PTR(values), RSTRING_LEN(values)));
  }

  data->request_headers_dirty = true;

  return mrb_nil_value();
}
} // namespace

namespace {
mrb_value request_set_header(mrb_state *mrb, mrb_value self) {
  return request_mod_header(mrb, self, true);
}
} // namespace

namespace {
mrb_value request_add_header(mrb_state *mrb, mrb_value self) {
  return request_mod_header(mrb, self, false);
}
} // namespace

namespace {
mrb_value request_clear_headers(mrb_state *mrb, mrb_value self) {
  auto data = static_cast<MRubyAssocData *>(mrb->ud);
  auto downstream = data->downstream;

  check_phase(mrb, data->phase, PHASE_REQUEST);

  downstream->clear_request_headers();

  return mrb_nil_value();
}
} // namespace

void init_request_class(mrb_state *mrb, RClass *module) {
  auto request_class =
      mrb_define_class_under(mrb, module, "Request", mrb->object_class);

  mrb_define_method(mrb, request_class, "initialize", request_init,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, request_class, "http_version_major",
                    request_get_http_version_major, MRB_ARGS_NONE());
  mrb_define_method(mrb, request_class, "http_version_minor",
                    request_get_http_version_minor, MRB_ARGS_NONE());
  mrb_define_method(mrb, request_class, "method", request_get_method,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, request_class, "method=", request_set_method,
                    MRB_ARGS_REQ(1));
  mrb_define_method(mrb, request_class, "authority", request_get_authority,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, request_class, "authority=", request_set_authority,
                    MRB_ARGS_REQ(1));
  mrb_define_method(mrb, request_class, "scheme", request_get_scheme,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, request_class, "scheme=", request_set_scheme,
                    MRB_ARGS_REQ(1));
  mrb_define_method(mrb, request_class, "path", request_get_path,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, request_class, "path=", request_set_path,
                    MRB_ARGS_REQ(1));
  mrb_define_method(mrb, request_class, "headers", request_get_headers,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, request_class, "add_header", request_add_header,
                    MRB_ARGS_REQ(2));
  mrb_define_method(mrb, request_class, "set_header", request_set_header,
                    MRB_ARGS_REQ(2));
  mrb_define_method(mrb, request_class, "clear_headers", request_clear_headers,
                    MRB_ARGS_NONE());
}

} // namespace mruby

} // namespace shrpx
