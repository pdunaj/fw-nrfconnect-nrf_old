/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#ifndef _EVENT_MANAGER_H_
#define _EVENT_MANAGER_H_

/**
 * @brief Event Manager
 * @defgroup event_manager Event Manager
 * @{
 */

#include <zephyr.h>
#include <zephyr/types.h>
#include <misc/__assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _SUBS_PRIO_FIRST  0
#define _SUBS_PRIO_NORMAL 1
#define _SUBS_PRIO_FINAL  2

#define SUBS_PRIO_MIN    _SUBS_PRIO_FIRST
#define SUBS_PRIO_MAX    _SUBS_PRIO_FINAL

#define SUBS_PRIO_COUNT (SUBS_PRIO_MAX - SUBS_PRIO_MIN + 1)

#define _SUBS_PRIO_ID(level) _CONCAT(_CONCAT(_prio, level), _)


struct event_header {
	sys_dlist_t node;

	const void *type_id;
	s64_t timestamp;
};

struct event_listener {
	const char *name;

	bool (*notification)(const struct event_header *eh);
};

struct event_subscriber {
	const struct event_listener *listener;
};

struct event_type {
	const char			*name;
	const struct event_subscriber	*subs_start[SUBS_PRIO_COUNT];
	const struct event_subscriber	*subs_stop[SUBS_PRIO_COUNT];

	void (*print_event)(const struct event_header *eh);
};

extern const struct event_listener __start_event_listeners[];
extern const struct event_listener __stop_event_listeners[];

extern const struct event_type __start_event_types[];
extern const struct event_type __stop_event_types[];

/**
 * @def EVENT_LISTENER
 *
 * @brief Create event listener object.
 *
 * @param name             Module name.
 * @param notification_fn  Pointer to event notification function.
 */
#define EVENT_LISTENER(lname, notification_fn)					\
	const struct event_listener _CONCAT(__event_listener_, lname) __used	\
	__attribute__((__section__("event_listeners"))) = {			\
		.name = STRINGIFY(lname),					\
		.notification = (notification_fn),				\
	}


#define _EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, prio)	_CONCAT(_CONCAT(event_subscribers_, ename), prio)

#define _EVENT_SUBSCRIBERS_SECTION_NAME(ename, prio)	STRINGIFY(_EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, prio))

#define _EVENT_SUBSCRIBERS_EMPTY(ename, prio)								\
	const struct {} _CONCAT(_EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, prio), empty)			\
	__attribute__((__section__(STRINGIFY(_EVENT_SUBSCRIBERS_SECTION_NAME(ename, prio))))) = {};

#define _EVENT_SUBSCRIBERS_START(ename, prio)	_CONCAT(__start_, _EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, prio))

#define _EVENT_SUBSCRIBERS_STOP(ename, prio)	_CONCAT(__stop_,  _EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, prio))

#define _EVENT_SUBSCRIBERS_DECLARE(ename)										\
	extern const struct event_subscriber _EVENT_SUBSCRIBERS_START(ename, _SUBS_PRIO_ID(_SUBS_PRIO_FIRST))[];	\
	extern const struct event_subscriber _EVENT_SUBSCRIBERS_STOP(ename,  _SUBS_PRIO_ID(_SUBS_PRIO_FIRST))[];	\
	extern const struct event_subscriber _EVENT_SUBSCRIBERS_START(ename, _SUBS_PRIO_ID(_SUBS_PRIO_NORMAL))[];	\
	extern const struct event_subscriber _EVENT_SUBSCRIBERS_STOP(ename,  _SUBS_PRIO_ID(_SUBS_PRIO_NORMAL))[];	\
	extern const struct event_subscriber _EVENT_SUBSCRIBERS_START(ename, _SUBS_PRIO_ID(_SUBS_PRIO_FINAL))[];	\
	extern const struct event_subscriber _EVENT_SUBSCRIBERS_STOP(ename,  _SUBS_PRIO_ID(_SUBS_PRIO_FINAL))[];

#define _EVENT_SUBSCRIBERS_DEFINE(ename)					\
	_EVENT_SUBSCRIBERS_EMPTY(ename, _SUBS_PRIO_ID(_SUBS_PRIO_FIRST))	\
	_EVENT_SUBSCRIBERS_EMPTY(ename, _SUBS_PRIO_ID(_SUBS_PRIO_NORMAL))	\
	_EVENT_SUBSCRIBERS_EMPTY(ename, _SUBS_PRIO_ID(_SUBS_PRIO_FINAL))

#define _EVENT_SUBSCRIBE(lname, ename, prio)								\
	const struct event_subscriber _CONCAT(_CONCAT(__event_subscriber_, ename), lname) __used	\
	__attribute__((__section__(_EVENT_SUBSCRIBERS_SECTION_NAME(ename, prio)))) = {			\
		.listener = &_CONCAT(__event_listener_, lname),						\
	}

#define EVENT_SUBSCRIBE_EARLY(lname, ename)	_EVENT_SUBSCRIBE(lname, ename, _SUBS_PRIO_ID(_SUBS_PRIO_FIRST))

#define EVENT_SUBSCRIBE(lname, ename)		_EVENT_SUBSCRIBE(lname, ename, _SUBS_PRIO_ID(_SUBS_PRIO_NORMAL))

#define EVENT_SUBSCRIBE_FINAL(lname, ename)							\
	_EVENT_SUBSCRIBE(lname, ename, _SUBS_PRIO_ID(_SUBS_PRIO_FINAL));			\
	const struct {} _CONCAT(_CONCAT(__event_subscriber_, ename), final_sub_redefined) = {}


#define _EVENT_ID(ename) (&_CONCAT(__event_type_, ename))

#define _EVENT_ALLOCATOR_FN(ename)					\
	static inline struct ename *_CONCAT(new_, ename)(void)		\
	{								\
		struct ename *event = k_malloc(sizeof(*event));		\
		if (!event) {						\
			return NULL;					\
		}							\
		event->header.type_id = _EVENT_ID(ename);		\
		event->header.timestamp = k_uptime_get();		\
		return event;						\
	}

#define _EVENT_CASTER_FN(ename)									\
	static inline struct ename *_CONCAT(cast_, ename)(const struct event_header *eh)	\
	{											\
		struct ename *event = NULL;							\
		if (eh->type_id == _EVENT_ID(ename)) {						\
			event = CONTAINER_OF(eh, struct ename, header);				\
		}										\
		return event;									\
	}

#define _EVENT_TYPECHECK_FN(ename) \
	static inline bool _CONCAT(is_, ename)(const struct event_header *eh)	\
	{									\
		return (eh->type_id == _EVENT_ID(ename));			\
	}

#define EVENT_TYPE_DECLARE(ename)					\
	extern const struct event_type _CONCAT(__event_type_, ename);	\
	_EVENT_SUBSCRIBERS_DECLARE(ename);				\
	_EVENT_ALLOCATOR_FN(ename);					\
	_EVENT_CASTER_FN(ename);					\
	_EVENT_TYPECHECK_FN(ename)

#define EVENT_TYPE_DEFINE(ename, print_fn)										\
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
	}

#define ASSERT_EVENT_ID(id) \
	__ASSERT_NO_MSG((id >= __start_event_types) && (id < __stop_event_types))

void _event_submit(struct event_header *event);

#define EVENT_SUBMIT(event) _event_submit(&event->header)


/** Initialize event manager.
 *
 * @return Zero if successful.
 */
int event_manager_init(void);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _EVENT_MANAGER_H_ */
