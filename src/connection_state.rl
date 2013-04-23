%%{
	machine ConnectionState;

	import "events.h";

	connection = (
		start: (
			OPEN @parse_init -> parse_continue
		),

		parse_continue: (
			PARSE @parse_exec -> parse_continue				|
			PARSE_DONE @read_request_method -> request_read	|
			PARSE_ERROR	@send_request_invalid -> close 		|
 			CLOSE @cleanup -> final
		),

		request_read: (
			GET_REQUEST @validate_get -> get_validating		|
			INV_REQUEST @send_request_invalid -> close
		),

		get_validating: (
			GET_VALID @send_get_response -> close			|
			GET_NOT_VALID @send_request_invalid -> close	|
			GET_NOT_FOUND @send_404_response -> close
		)

		close: (
			CLOSE @cleanup -> final
		)
	);

	main := (connection)*;
}%%