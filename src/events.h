#ifndef events_h
#define events_h

typedef enum StateEvent {
	OPEN=9,
	GET=10,
	CLOSE=11,
	INV_REQUEST=12,
	PARSE=13,
	PARSE_DONE=14,
	PARSE_ERROR=15,
	GET_VALID=16,
	GET_NOT_VALID=17,
	KEEPALIVE=18
} StateEvent;

#endif /* events_h */