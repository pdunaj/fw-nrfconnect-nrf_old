/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <misc/dlist.h>
#include <misc/printk.h>
#include <event_manager.h>

static void event_processor_fn(struct k_work *work);

K_WORK_DEFINE(event_processor, event_processor_fn);

static sys_dlist_t eventq = SYS_DLIST_STATIC_INIT(&eventq);

static u16_t *profiler_event_ids;

static void event_processor_fn(struct k_work *work)
{
	sys_dlist_t events;

	/* Make current event list local */
	unsigned int flags = irq_lock();

if (sys_dlist_is_empty(&eventq)) {
		irq_unlock(flags);
		return;
	}

	events = eventq;
	events.next->prev = &events;
	events.prev->next = &events;
	sys_dlist_init(&eventq);

	irq_unlock(flags);


	/* Traverse the list of events */
	struct event_header *eh;

	while (NULL != (eh = CONTAINER_OF(sys_dlist_get(&events),
					  struct event_header,
					  node))) {

		ASSERT_EVENT_ID(eh->type_id);

		const struct event_type *et = eh->type_id;

		if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_TRACE_EVENT_EXECUTION)) {
			struct log_event_buf buf;
			event_log_start(&buf);
			event_log_add_mem_address(eh, &buf);
			/* Event execution start in event manager has next id after all the events */
			event_log_send(profiler_event_ids[__stop_event_types - __start_event_types +1], &buf); 
		}
		if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_SHOW_EVENTS)) {
			printk("e: %s ", et->name);
			if (et->print_event) {
				et->print_event(eh);
			}

			printk("\n");
		}

		bool consumed = false;

		for (size_t prio = SUBS_PRIO_MIN;
		     (prio <= SUBS_PRIO_MAX) && !consumed;
		     prio++) {
			for (const struct event_subscriber *es =
					et->subs_start[prio];
			     (es != et->subs_stop[prio]) && !consumed;
			     es++) {

				__ASSERT_NO_MSG(es != NULL);

				const struct event_listener *el = es->listener;

				__ASSERT_NO_MSG(el != NULL);
				__ASSERT_NO_MSG(el->notification != NULL);

				consumed = el->notification(eh);

				if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_SHOW_EVENTS) &&
				    IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_SHOW_EVENT_HANDLERS)) {
					printk("|\t%s notified%s\n",
						el->name,
						(consumed)?(" (event consumed)"):(""));
				}
			}
		}
		if (IS_ENABLED(DESKTOP_EVENT_MANAGER_TRACE_EVENT_EXECUTION)) {
			struct log_event_buf buf;
			event_log_start(&buf);
			event_log_add_mem_address(eh, &buf);
			/* Event execution end in event manager has next id after all the events adn event execution start */
			event_log_send(profiler_event_ids[__stop_event_types - __start_event_types +2], &buf);
		}		
		k_free(eh);
	}

	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_SHOW_EVENTS) &&
	    IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_SHOW_EVENT_HANDLERS)) {
		printk("|\n\n");
	}
}

void _event_submit(struct event_header *eh)
{
	unsigned int flags = irq_lock();

	sys_dlist_append(&eventq, &eh->node);

	irq_unlock(flags);

	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_PROFILER_ENABLED)) {	
		const struct event_type *et = eh->type_id;
		if(et->log_args) {		
			struct log_event_buf buf;
			event_log_start(&buf);
			if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_TRACE_EVENT_EXECUTION)) {
				event_log_add_mem_address(eh, &buf);
			}
			et->log_args(eh, &buf);
			event_log_send(profiler_event_ids[et - __start_event_types], &buf);
		}			
	}
	k_work_submit(&event_processor);
}

static void event_manager_show_listeners(void)
{
	printk("Registered Listeners:\n");
	for (const struct event_listener *el = __start_event_listeners;
	     el != __stop_event_listeners;
	     el++) {
		__ASSERT_NO_MSG(el != NULL);
		printk("|\t[L:%s]\n", el->name);
	}
	printk("|\n\n");
}

static void event_manager_show_subscribers(void)
{
	printk("Registered Subscribers:\n");
	for (const struct event_type *et = __start_event_types;
	     (et != NULL) && (et != __stop_event_types);
	     et++) {

		bool is_subscribed = false;

		for (size_t prio = SUBS_PRIO_MIN;
		     prio <= SUBS_PRIO_MAX;
		     prio++) {
			for (const struct event_subscriber *es =
					et->subs_start[prio];
			     es != et->subs_stop[prio];
			     es++) {

				__ASSERT_NO_MSG(es != NULL);
				const struct event_listener *el = es->listener;

				__ASSERT_NO_MSG(el != NULL);
				printk("|\tprio:%u\t[E:%s] -> [L:%s]\n", prio,
						et->name, el->name);

				is_subscribed = true;
			}
		}

		if (!is_subscribed) {
			printk("|\t[E:%s] has no subscribers\n", et->name);
		}
		printk("|\n");
	}
	printk("\n");
}

static void register_predefined_events()
{
	u16_t profiler_event_id;
	//event execution start event after last event
	profiler_event_id = profiler_register_event_type("event_processing_start", NULL, NULL, 0);
	profiler_event_ids[__stop_event_types - __start_event_types + 1] = profiler_event_id;
	
	//event execution stop event 
	profiler_event_id = profiler_register_event_type("event_processing_end", NULL, NULL, 0);
	profiler_event_ids[__stop_event_types - __start_event_types + 2] = profiler_event_id;
}

static void register_events()
{
	for (const struct event_type *et = __start_event_types;
	     (et != NULL) && (et != __stop_event_types);
	     et++) {
		if (et->log_info) {
			u16_t profiler_event_id;
			profiler_event_id = profiler_register_event_type(et->name, 
				et->log_info->log_args_labels, et->log_info->log_args_types, 
				et->log_info->log_args_cnt);
			profiler_event_ids[et - __start_event_types] = profiler_event_id;
		}
 	}
	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_TRACE_EVENT_EXECUTION)) {
		register_predefined_events();
	}
}

int event_manager_init(void)
{
	profiler_event_ids = k_malloc((__stop_event_types - __start_event_types + 2) * sizeof(u16_t));
	if (!profiler_event_ids) {
		printk("event_manager: memory allocation failed \n");
		return -1;
	}
	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_PROFILER_ENABLED)) {	
		profiler_init();
		register_events();
	}
	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_SHOW_LISTENERS)) {
		event_manager_show_listeners();
		event_manager_show_subscribers();
	}
	return 0;
}

