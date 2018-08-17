/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

/** @file
 * @brief Event manager private header.
 *
 * Although these defines are globally visible they must not be used directly.
 */

#ifndef _EVENT_MANAGER_PRIV_H_
#define _EVENT_MANAGER_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif


/* There are 3 levels of priorities defining an order at which event listeners
 * are notified about incoming events.
 */

#define _SUBS_PRIO_FIRST  0
#define _SUBS_PRIO_NORMAL 1
#define _SUBS_PRIO_FINAL  2


/* Convenience macros generating section names. */

#define _SUBS_PRIO_ID(level) _CONCAT(_CONCAT(_prio, level), _)

#define _EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, prio)	_CONCAT(_CONCAT(event_subscribers_, ename), prio)

#define _EVENT_SUBSCRIBERS_SECTION_NAME(ename, prio)	STRINGIFY(_EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, prio))


/* Declare a zero-length subscriber. */
#define _EVENT_SUBSCRIBERS_EMPTY(ename, prio)								\
	const struct {} _CONCAT(_EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, prio), empty)			\
	__attribute__((__section__(STRINGIFY(_EVENT_SUBSCRIBERS_SECTION_NAME(ename, prio))))) = {};


/* Convenience macros generating section start and stop markers. */

#define _EVENT_SUBSCRIBERS_START(ename, prio)	_CONCAT(__start_, _EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, prio))

#define _EVENT_SUBSCRIBERS_STOP(ename, prio)	_CONCAT(__stop_,  _EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, prio))


#define _EVENT_SUBSCRIBERS_DECLARE(ename)										\
	extern const struct event_subscriber _EVENT_SUBSCRIBERS_START(ename, _SUBS_PRIO_ID(_SUBS_PRIO_FIRST))[];	\
	extern const struct event_subscriber _EVENT_SUBSCRIBERS_STOP(ename,  _SUBS_PRIO_ID(_SUBS_PRIO_FIRST))[];	\
	extern const struct event_subscriber _EVENT_SUBSCRIBERS_START(ename, _SUBS_PRIO_ID(_SUBS_PRIO_NORMAL))[];	\
	extern const struct event_subscriber _EVENT_SUBSCRIBERS_STOP(ename,  _SUBS_PRIO_ID(_SUBS_PRIO_NORMAL))[];	\
	extern const struct event_subscriber _EVENT_SUBSCRIBERS_START(ename, _SUBS_PRIO_ID(_SUBS_PRIO_FINAL))[];	\
	extern const struct event_subscriber _EVENT_SUBSCRIBERS_STOP(ename,  _SUBS_PRIO_ID(_SUBS_PRIO_FINAL))[];


/* Macro defining empty subscribers on each priority level.
 * Each event type keeps an array of subscribers for every priority level.
 * It can happen that for a given priority no subscriber will be registered.
 * This macro declares zero-length subscriber that will cause required section
 * to be generated by the linker. If no subscriber is registered at this
 * level section will remain empty.
 */
#define _EVENT_SUBSCRIBERS_DEFINE(ename)					\
	_EVENT_SUBSCRIBERS_EMPTY(ename, _SUBS_PRIO_ID(_SUBS_PRIO_FIRST))	\
	_EVENT_SUBSCRIBERS_EMPTY(ename, _SUBS_PRIO_ID(_SUBS_PRIO_NORMAL))	\
	_EVENT_SUBSCRIBERS_EMPTY(ename, _SUBS_PRIO_ID(_SUBS_PRIO_FINAL))


/* Subscribe a listener to an event. */
#define _EVENT_SUBSCRIBE(lname, ename, prio)								\
	const struct event_subscriber _CONCAT(_CONCAT(__event_subscriber_, ename), lname) __used	\
	__attribute__((__section__(_EVENT_SUBSCRIBERS_SECTION_NAME(ename, prio)))) = {			\
		.listener = &_CONCAT(__event_listener_, lname),						\
	}


/* Pointer to event type definition is used as event type identifier. */
#define _EVENT_ID(ename) (&_CONCAT(__event_type_, ename))


/* Macro generates a function of name new_ename where ename is provided as
 * an argument. Allocator function is used to create an event of the given
 * ename type.
 */
#define _EVENT_ALLOCATOR_FN(ename)					\
	static inline struct ename *_CONCAT(new_, ename)(void)		\
	{								\
		struct ename *event = k_malloc(sizeof(*event));		\
		if (!event) {						\
			return NULL;					\
		}							\
		event->header.type_id = _EVENT_ID(ename);		\
		return event;						\
	}


/* Macro generates a function of name cast_ename where ename is provided as
 * an argument. Casting function is used to convert event_header pointer
 * into pointer to event matching the given ename type.
 */
#define _EVENT_CASTER_FN(ename)									\
	static inline struct ename *_CONCAT(cast_, ename)(const struct event_header *eh)	\
	{											\
		struct ename *event = NULL;							\
		if (eh->type_id == _EVENT_ID(ename)) {						\
			event = CONTAINER_OF(eh, struct ename, header);				\
		}										\
		return event;									\
	}


/* Macro generates a function of name is_ename where ename is provided as
 * an argument. Typecheck function is used to check if pointer to event_header
 * belongs to the event matching the given ename type.
 */
#define _EVENT_TYPECHECK_FN(ename) \
	static inline bool _CONCAT(is_, ename)(const struct event_header *eh)	\
	{									\
		return (eh->type_id == _EVENT_ID(ename));			\
	}


/* Declarations and definitions - for more details refer to public API. */

#ifdef CONFIG_DESKTOP_EVENT_MANAGER_TRACE_EVENT_EXECUTION

#define _ARG_LABELS_DEFINE(...) \
	const static char *log_arg_labels[] __used = \
	{"mem_address", __VA_ARGS__};

#define _ARG_TYPES_DEFINE(...) \
	const static enum profiler_arg log_arg_types[] __used = \
	 {PROFILER_ARG_U32, __VA_ARGS__};

#else	

#define _ARG_LABELS_DEFINE(...) \
	const static char *log_arg_labels[] __used = \
	{__VA_ARGS__};

#define _ARG_TYPES_DEFINE(...) \
	const static enum profiler_arg log_arg_types[] __used = \
	{__VA_ARGS__};

#endif	


#define _EVENT_MEM_ALLOCATE_PROFILER_ID(ename) 			\
	static u16_t ename __used                           	\
	__attribute__((__section__("profiler_ids")));		\
		

#define _EVENT_INFO_DEFINE(ename, types, labels, log_arg_func)			\
	_ARG_LABELS_DEFINE(labels)					 	\
	_ARG_TYPES_DEFINE(types)						\
        const static struct event_info _CONCAT(ename, _info) __used       \
        __attribute__((__section__("event_infos"))) = {                         \
		                .log_arg_fn      = log_arg_func,    	        \
		                .log_arg_cnt     = ARRAY_SIZE(log_arg_labels), 	\
	       			.log_arg_labels  = log_arg_labels,     		\
				.log_arg_types	 = log_arg_types                \
		        }                                                                                                           
          

#define _EVENT_LISTENER(lname, notification_fn)					\
	const struct event_listener _CONCAT(__event_listener_, lname) __used	\
	__attribute__((__section__("event_listeners"))) = {			\
		.name = STRINGIFY(lname),					\
		.notification = (notification_fn),				\
	}


#define _EVENT_TYPE_DECLARE(ename)					\
	extern const struct event_type _CONCAT(__event_type_, ename);	\
	_EVENT_SUBSCRIBERS_DECLARE(ename);				\
	_EVENT_ALLOCATOR_FN(ename);					\
	_EVENT_CASTER_FN(ename);					\
	_EVENT_TYPECHECK_FN(ename)


#define _EVENT_TYPE_DEFINE(ename, print_fn, ev_info_struct)								\
	_EVENT_SUBSCRIBERS_DEFINE(ename);										\
	const struct event_type _CONCAT(__event_type_, ename) __used							\
	__attribute__((__section__("event_types"))) = {									\
		.name				= STRINGIFY(ename),							\
		.subs_start	= {											\
			[_SUBS_PRIO_FIRST]	= _EVENT_SUBSCRIBERS_START(ename, _SUBS_PRIO_ID(_SUBS_PRIO_FIRST)),	\
			[_SUBS_PRIO_NORMAL]	= _EVENT_SUBSCRIBERS_START(ename, _SUBS_PRIO_ID(_SUBS_PRIO_NORMAL)),	\
			[_SUBS_PRIO_FINAL]	= _EVENT_SUBSCRIBERS_START(ename, _SUBS_PRIO_ID(_SUBS_PRIO_FINAL)),	\
		},													\
		.subs_stop	= {											\
			[_SUBS_PRIO_FIRST]	= _EVENT_SUBSCRIBERS_STOP(ename, _SUBS_PRIO_ID(_SUBS_PRIO_FIRST)),	\
			[_SUBS_PRIO_NORMAL]	= _EVENT_SUBSCRIBERS_STOP(ename, _SUBS_PRIO_ID(_SUBS_PRIO_NORMAL)),	\
			[_SUBS_PRIO_FINAL]	= _EVENT_SUBSCRIBERS_STOP(ename, _SUBS_PRIO_ID(_SUBS_PRIO_FINAL)),	\
		},													\
		.print_event			= print_fn,								\
		.ev_info			= ev_info_struct,							\
	}


#ifdef __cplusplus
}
#endif

#endif /* _EVENT_MANAGER_PRIV_H_ */
