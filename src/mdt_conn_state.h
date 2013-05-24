#ifndef events_h
#define events_h

typedef enum StateEvent {
	OPEN = 10,
	PARSE = 11,
	PARSE_DONE = 12,
	PARSE_ERROR = 13,
	GET_REQUEST = 14,
	INV_REQUEST = 15,
	GET_VALID = 16,
	GET_NOT_VALID = 17,
	GET_NOT_FOUND = 18,
	CLOSE = 19,
	DONE = 20
} StateEvent;

#endif /* events_h */