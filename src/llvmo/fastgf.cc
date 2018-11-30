#include <clasp/core/foundation.h>
#include <clasp/core/lispStream.h> // write_bf_stream
#include <clasp/core/evaluator.h> // funcall
#include <clasp/core/core.h>
#include <clasp/core/object.h>
#include <clasp/core/array.h>
#include <clasp/core/instance.h>
#include <clasp/core/wrappedPointer.h>
#include <clasp/core/funcallableInstance.h>
#include <clasp/core/derivableCxxObject.h>
#include <clasp/gctools/gc_interface.h>
#include <clasp/llvmo/intrinsics.h>

#if 0 // DEBUGGING
#define NO_UNWIND_BEGIN_BUILTINS() NO_UNWIND_BEGIN()
#define NO_UNWIND_END_BUILTINS() NO_UNWIND_END()
#define ENSURE_VALID_OBJECT_BUILTINS(x) ENSURE_VALID_OBJECT(x)
#else
#define NO_UNWIND_BEGIN_BUILTINS()
#define NO_UNWIND_END_BUILTINS()
#define ENSURE_VALID_OBJECT_BUILTINS(x) x
#endif

#define LINKAGE __attribute__ ((visibility ("default")))

#define BUILTIN_ATTRIBUTES __attribute__((always_inline))


extern "C" {

#if !( (FIXNUM0_TAG<GENERAL_TAG) && (GENERAL_TAG<CHARACTER_TAG) && (CHARACTER_TAG<CONS_TAG) && (CONS_TAG<FIXNUM1_TAG) && (FIXNUM1_TAG<VASLIST_TAG) && (VASLIST_TAG<SINGLE_FLOAT_TAG) && (SINGLE_FLOAT_TAG<ZERO_TAG_MASK))
#error "The tag values do not match the order needed by cc_read_stamp"
#endif
 
BUILTIN_ATTRIBUTES int64_t cc_read_stamp(void* tagged_pointer)
{
  uintptr_t tag = reinterpret_cast<uintptr_t>(tagged_pointer)&gctools::tag_mask;
  int64_t stamp;
  switch (tag) {
  case FIXNUM0_TAG:
      return gctools::STAMP_FIXNUM;
  case GENERAL_TAG: {
  // do more stuff to get the stamp
    core::General_O* client_ptr = reinterpret_cast<core::General_O*>(gctools::untag_general<core::T_O*>(reinterpret_cast<core::T_O*>(tagged_pointer)));
    const gctools::Header_s& header = *reinterpret_cast<const gctools::Header_s *>(ClientPtrToBasePtr(client_ptr));
    uint64_t stamp = header.stamp();
    if (stamp == gctools::STAMP_core__Instance_O ||
        stamp == gctools::STAMP_core__FuncallableInstance_O ||
        stamp == global_TheClassRep_stamp ) {
      core::Instance_O* instance_ptr = reinterpret_cast<core::Instance_O*>(client_ptr);
      core::SimpleVector_O* rack = reinterpret_cast<core::SimpleVector_O*>(gctools::untag_general<core::T_O*>(instance_ptr->_Rack.raw_()));
      return (*rack)[0].unsafe_fixnum();
    } else if ( stamp == gctools::STAMP_core__WrappedPointer_O ) {
      core::WrappedPointer_O* wrapped_ptr = reinterpret_cast<core::WrappedPointer_O*>(client_ptr);
      return wrapped_ptr->Stamp_;
    } else if ( stamp == gctools::STAMP_core__DerivableCxxObject_O ) {
      core::DerivableCxxObject_O* derivable_cxx_object_ptr = reinterpret_cast<core::DerivableCxxObject_O*>(client_ptr);
      return derivable_cxx_object_ptr->get_stamp_();
    } else {
      return stamp;
    }
  }
  case CHARACTER_TAG:
      return gctools::STAMP_CHARACTER;
  case CONS_TAG:
      return gctools::STAMP_CONS;
  case FIXNUM1_TAG:
      return gctools::STAMP_FIXNUM;
  case VASLIST_TAG:
      return gctools::STAMP_VA_LIST_S;
  case SINGLE_FLOAT_TAG:
      return gctools::STAMP_SINGLE_FLOAT;
  }
  return 123456;
}


};

extern "C" {
BUILTIN_ATTRIBUTES gctools::return_type cc_dispatch_miss(core::T_O* tgf, core::T_O* tgf_vaslist)
{
  core::FuncallableInstance_sp gf((gctools::Tagged)tgf);
  core::VaList_sp gf_vaslist((gctools::Tagged)tgf_vaslist);
  core::T_mv result = core::eval::funcall(clos::_sym_dispatch_miss,gf,gf_vaslist);
#ifdef DEBUG_GFDISPATCH
  printf("%s:%d  Returning from cc_dispatch_miss\n", __FILE__, __LINE__ );
#endif
  return result.as_return_type();
}

BUILTIN_ATTRIBUTES core::T_O* cc_dispatch_slot_reader_index(size_t index, core::T_O* tinstance) {
  core::Instance_sp instance((gctools::Tagged)tinstance);
  core::T_sp value = low_level_instanceRef(instance->_Rack,index);
  return value.raw_();
}

BUILTIN_ATTRIBUTES core::T_O* cc_dispatch_slot_reader_cons(core::T_O* toptinfo) {
  core::SimpleVector_sp optinfo((gctools::Tagged)toptinfo);
  core::Cons_sp cons = gc::As_unsafe<core::Cons_sp>((*optinfo)[OPTIMIZED_SLOT_INDEX_INDEX]);
  core::T_sp value = CONS_CAR(cons);
  return value.raw_();
}

BUILTIN_ATTRIBUTES void cc_vaslist_end(core::T_O* tvaslist) {
  core::VaList_sp vaslist((gctools::Tagged)tvaslist);
  va_end(vaslist->_Args);
}


BUILTIN_ATTRIBUTES core::T_O* cc_dispatch_slot_writer_cons(core::T_O* tvalue, core::T_O* toptinfo) {
  core::SimpleVector_sp optinfo((gctools::Tagged)toptinfo);
  core::Cons_sp cons = gc::As_unsafe<core::Cons_sp>((*optinfo)[OPTIMIZED_SLOT_INDEX_INDEX]);
  core::T_sp value((gctools::Tagged)tvalue);
  CONS_CAR(cons) = value;
  return value.raw_();
}

BUILTIN_ATTRIBUTES core::T_O* cc_dispatch_slot_writer_index(core::T_O* tvalue, size_t index, core::T_O* tinstance) {
  core::T_sp value((gctools::Tagged)tvalue);
  core::Instance_sp instance((gctools::Tagged)tinstance);
  low_level_instanceSet(instance->_Rack,index,value);
  return value.raw_();
}



BUILTIN_ATTRIBUTES core::T_O* cc_bound_or_error(core::T_O* toptimized_slot_reader, core::T_O* tinstance, core::T_O* tvalue) {
  core::T_sp value((gctools::Tagged)tvalue);
  if (value.unboundp()) {
    core::Instance_sp instance((gctools::Tagged)tinstance);
    core::T_sp optimized_slot_info((gctools::Tagged)toptimized_slot_reader);
    return llvmo::intrinsic_slot_unbound(optimized_slot_info,instance).raw_();
  }
  return value.raw_();
}


BUILTIN_ATTRIBUTES core::T_O* cc_fastgf_nil() {
  return _Nil<core::T_O>().raw_();
}


BUILTIN_ATTRIBUTES core::T_O* cc_rewind_vaslist(core::Vaslist* vaslist, va_list va_args, void** register_save_areaP)
{
  va_copy(vaslist->_Args,va_args);
  LCC_REWIND_VA_LIST(vaslist->_Args,register_save_areaP);
  vaslist->remaining_nargs() = (uintptr_t)register_save_areaP[1];
  return gctools::tag_vaslist<core::T_O*>(vaslist);
}

BUILTIN_ATTRIBUTES void cc_dispatch_debug(int msg_id, uintptr_clasp_t val)
{
  // The msg_id switch values correspond to values passed from cmpgf.lsp
  //   The values mean:
  //         0 - print the argument as an integer step index
  //         1 - Print the value as a integer
  //         2 - print the value as a tag
  //         3 - print the value as a tagged pointer to a Vaslist object
  //         4 - print the value as a pointer
  //         5 - print the contents of the va_list pointed to by the value
  //         6 - print the value as a stamp
  //         7 - print the value as a pointer to a dispatch function
  switch (msg_id) {
  case 0:
      core::write_bf_stream(BF("Step %d\n") % val);
//      printf("%s:%d    cc_dispatch_debug step %d\n", __FILE__, __LINE__, val );
      break;
  case 1:
      core::write_bf_stream(BF("Arg val[%d]") % val);
//      printf("%s:%d    cc_dispatch_debug arg val[%d]\n", __FILE__, __LINE__, val );
      break;
  case 2:
      core::write_bf_stream(BF(" tag = %d\n") % val); 
//      printf("%s:%d    cc_dispatch_debug tag [%d]\n", __FILE__, __LINE__, val );
     break;
  case 3: {
    core::VaList_sp vls((gc::Tagged)val);
//    printf("%s:%d    vaList_sp.raw_() = %p\n", __FILE__, __LINE__, vls.raw_());
    core::write_bf_stream(BF("Arg VaList_sp.raw_() = %p list -> %s\n") % (void*)vls.raw_() % _rep_(vls) );
    dump_Vaslist_ptr(&*vls);
    break;
  }
  case 4: {
//      printf("%s:%d     ptr: %p\n", __FILE__, __LINE__, (void*)val);
      core::write_bf_stream(BF("Ptr: %p\n") % (void*)val );
  }
      break;
  case 5: {
//      printf("%s:%d     ptr: %p\n", __FILE__, __LINE__, (void*)val);
      core::write_bf_stream(BF("va_list: %p\n") % (void*)val );
      void* dump_va_list_voidSTAR = (void*)&core::dump_va_list;
      typedef void (*fptr)(uintptr_t);
      fptr my_fptr = reinterpret_cast<fptr>(dump_va_list_voidSTAR);
      my_fptr(val);
      break;
  }
  case 6:
      core::write_bf_stream(BF("Argument stamp: %lu\n") % val);
      break;
  case 7:
      core::write_bf_stream(BF("Dispatch to: %p\n") % val);
      break;
  }
  fflush(stdout);
}

};
