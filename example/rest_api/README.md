## Example: Basic CRUD API

This example demonstrates a basic CRUD API using the following components:

* **PostgreSQL**
* **pog\_pool** (PostgreSQL connection pool)
* **jansson** (JSON parsing and serialization)

### Setup Instructions

1. **Header Files**
   Move all required header files into the `third_party/include` directory.

2. **macOS Users**
   When copying `libpq` headers, ensure they are placed under:

   ```
   third_party/include/postgresql
   ```

   This prefix is required on macOS, whereas most Linux distributions already structure it this way by default.
