# libmailwrap

**libmailwrap** is a lightweight C library that provides a simple
interface for sending emails.\
It acts as a wrapper around the standard **`/bin/mail`** utility,
allowing C applications to send email without handling SMTP or mail
protocols directly.

------------------------------------------------------------------------

## Features

-   Minimal, portable C API.
-   Relies on `/bin/mail` for actual message delivery.
-   Provides error handling via return codes and failure counters.
-   Enforces a maximum time of execution, to avoid hanging the
    calling program if `/bin/mail` hangs
-   Easy to embed into existing C projects.
-   Licensed under the **GNU LGPL v3 (or later)**.

------------------------------------------------------------------------

## Build and Install

Clone the repository and run:

``` sh
make
sudo make install
```

This will:

-   Build `libmailwrap.so.1.0` (shared library).
-   Install the header file (`LMW_send_email.h`) to
    `/usr/local/include`.
-   Install the library (`libmailwrap.so.1.0`) to `/usr/local/lib` and
    create a `libmailwrap.so` symlink.

### Dependencies

-   A working **`/bin/mail`** program (commonly provided by `mailutils`
    or `bsd-mailx`).
-   A C compiler (e.g., `gcc`).

------------------------------------------------------------------------

## Usage

### Include the library

``` c
#include <LMW_send_email.h>
```

### Link against it

Compile your program with:

``` sh
gcc -o myprog myprog.c -lmailwrap
```

------------------------------------------------------------------------

## Direct inclusion

It is also possible to simply include the library code
in your project: see the example program `LMW_send_mail_direct.c`.
This can be run as

``` sh
./LMW_send_email_direct "user@example.com" "Test Subject" "Hello world"
```

If `/bin/mail` is installed and properly configured, this will send an
email to the recipient.

------------------------------------------------------------------------

## API Reference

### `LMW_config`

The configuration struct used by the library. Initialize it before
sending:

``` c
LMW_config cfg;
LWM_config_init(&cfg);
```

Tracks internal state such as failure count.

------------------------------------------------------------------------

### `int LMW_send_email(LMW_config *cfg, const char *to, const char *subject, const char *body);`

Sends an email message via `/bin/mail`.

-   **to** -- recipient email address\
-   **subject** -- subject line\
-   **body** -- plain text body\
-   **cfg** -- pointer to a `LMW_config` struct

**Returns:** - `0` on success\
- Non-zero on error (also increments `cfg->failures`)
  see `LWM_send_mail.h` for return codes.

------------------------------------------------------------------------

## Platform Support

-   **Supported**: Unix-like systems (Linux, BSD, macOS) that provide
    `/bin/mail`.\
-   **Not supported**: Windows and other systems without `/bin/mail`.

If you need cross-platform support, you may have to provide your own
mail-sending backend or ensure `/bin/mail` is available in your
environment.

------------------------------------------------------------------------

## License

This project is licensed under the **GNU Lesser General Public License
v3.0 or later (LGPL-3.0-or-later)**.\
See [LICENSE](https://www.gnu.org/licenses/lgpl-3.0.html) for details.
