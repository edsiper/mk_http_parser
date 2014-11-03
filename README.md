# Monkey HTTP Parser

This is an ongoing work to replace the current HTTP parser in [Monkey HTTP Server](http://monkey-project.com). The existent parser in Monkey 'works', but it can be improved on many ways to deliver better performance.

This parser is developed with the following items in mind:

- HTTP/1.1 (1.0 of course too)
- It allows 3 states for a parsed request:
  - MK\_HTTP\_OK: it's OK, ready to be processed.
  - MK\_HTTP\_PENDING: there are some missing bytes, try later.
  - MK\_HTTP\_ERROR: something went wrong in the request.
- The parser can be executed as many times over a request context, it will use some offsets to avoid re-parsing previous text.
- It do not care about logic based on protocol specs, mostly grammar for the first row, headers and optional body. The only exception is when a _Content-Length_ header is defined and it's used to determinate when a request is completed.
- Avoid contexts switches as much as possible.
- Do not mess current Monkey internal structures (yet).
- Fast Headers lookup (very important).
- Include a test program to perform different validations and values check after parsing.

## Details

More details about the Server can be found on the main [Monkey Project](http://monkey-project.com) web site.

## License

The code on this repository is under the terms of the [Apache License v2.0](http://www.apache.org/licenses/LICENSE-2.0.html).

## Author

Eduardo Silva <eduardo@monkey.io>
