-- For uui_generate_v4
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- Create enum type only if it doesn't already exist
DO $$
BEGIN
  IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'status_enum') THEN
    CREATE TYPE status_enum AS ENUM ('active', 'inactive', 'pending');
  END IF;
END$$;

-- Create table only if it doesn't already exist
CREATE TABLE IF NOT EXISTS ExampleTable (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),

    -- Numeric types
    small_int_col SMALLINT,
    int_col INTEGER,
    big_int_col BIGINT,
    decimal_col DECIMAL(10, 2),
    numeric_col NUMERIC(15, 5),
    real_col REAL,
    double_col DOUBLE PRECISION,
    serial_col SERIAL,

    -- Character types
    char_col CHAR(10),
    varchar_col VARCHAR(255),
    text_col TEXT,

    -- Date/Time types
    date_col DATE,
    time_col TIME,
    timestamp_col TIMESTAMP,
    timestamptz_col TIMESTAMPTZ,

    -- Boolean
    boolean_col BOOLEAN,

    -- UUID (additional)
    another_uuid UUID,

    -- JSON
    json_col JSON,
    jsonb_col JSONB,

    -- Array
    int_array_col INTEGER[],
    text_array_col TEXT[],

    -- Enum
    status_col status_enum,

    -- Bytea (binary)
    file_col BYTEA
);
