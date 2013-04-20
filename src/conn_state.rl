%%{
	machine ConnectionState;

	import "events.h";

	connection = (
		start: ( OPEN @parse_exec -> parse_continue ),

		parse_continue: (
			PARSE @parse_exec -> parse_continue		|
			PARSE_DONE @read_request_method -> request_read	|
			PARSE_ERROR	@cleanup -> final
		),

		request_read: (
			GET @validate_get -> get_validating		|
			INV_REQUEST @request_not_implimented -> final	|
			CLOSE -> final
		),

		get_validating: (
			GET_VALID @send_response -> final	|
			GET_NOT_VALID @send_request_invalid -> final
		)

	) >parse_init %cleanup;

	main := (connection)*;
}%%