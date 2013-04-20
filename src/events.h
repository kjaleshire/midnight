#ifndef events_h
#define events_h

typedef enum StateEvent {
	GET=10,
	CLOSE=11,
	INV_REQUEST=12,
	PARSE=13,
	PARSE_DONE=14,
	GET_VALID=15,
	GET_NOT_VALID=16,
	KEEPALIVE=17,
} StateEvent;

#endif /* events_h */