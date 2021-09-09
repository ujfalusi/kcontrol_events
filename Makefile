kcontrol_events: kcontrol_events.o
	$(CC) $(LDFLAGS) -lasound kcontrol_events.o -o kcontrol_events
kcontrol_events.o: kcontrol_events.c
	$(CC) $(CFLAGS) -c kcontrol_events.c

# remove object files and executable when user executes "make clean"
clean:
	rm *.o kcontrol_events
