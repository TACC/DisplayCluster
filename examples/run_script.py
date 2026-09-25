from DC import DC
dc = DC('localhost')
el = dc.create_event_list('/usr/local/examples/script')
dc.run_events(el)

