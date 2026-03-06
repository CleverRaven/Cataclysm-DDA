#!/bin/sh
# Requires sed and sqlite3 binaries to be in your path.
# Your job is to download the "Download: Firearms Dealer Transactions" and "Download: Personal Transfers and Registrations" files from this website: https://web.archive.org/web/20240403062154/https://www.mass.gov/info-details/data-about-firearms-licensing-and-transactions and place the csv files in same folder with this script.

# Created based on https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/HOWTO_MASSAGE_MA_GUN_DATA.md

# Translate the date fields to ISO-8601
sed "s_\([^/,]*\)/\([^/,]*\)/\([^/,]*\),\(.*\)_20\3-\1-\2,\4_g" *.csv

sqlite3 db.sqlite <<EOF
.timer on
.mode csv

-- Create personal_transactions table with columns from Personal Transactions csv header row
.import 'Personal Transactions 01.01.04 -12.31.23.csv' personal_transactions

-- Create sales table with columns from first Firearms Dealer Transactions csv header row
.import 01.01.04-12.31.08.csv sales

-- Import rest of Firearms Dealer Transactions csv files into sales table
.import --skip 1 01.01.09-12.31.21.csv sales
.import --skip 1 01.01.22-12.31.22.csv sales
.import --skip 1 01.01.23-12.31.23.csv sales

-- Filter and rename the personal transactions columns while copying into a new table
CREATE TABLE gun_data AS SELECT "TRANSACTION DATE" as date, "BUYER STATE" as state, MAKE as make, MODEL as model, TYPE as type, "SERIAL NUMBER" as serial, "WEAPON SIZE" as ammo_type FROM personal_transactions;

-- Filter and rename the sales columns while coping into the just created table
INSERT INTO gun_data SELECT "DATE OF TRANSACTION" as date, "BUYER STATE" as state, MAKE as make, MODEL as model, "WEAPON TYPE" as type, "SERIAL NUMBER" as serial, "WEAPON SIZE" as ammo_type FROM sales;

-- Drop the intermediate tables
DROP TABLE sales; DROP TABLE personal_transactions; VACUUM;

-- Deduplicate the "not available" values for serial number.
.read fix_serial_numbers.sql

-- Remove empty lines
.read remove_empty_entries.sql

-- Deduplicate rows with matching serial fields, with latest date winning
CREATE INDEX gun_data_idx_deduplication_query ON gun_data(serial, date); DELETE FROM gun_data WHERE rowid IN (SELECT q1.rowid from gun_data q1 JOIN gun_data q2 on q1.serial = q2.serial WHERE q1.serial != "NONE" AND q1.date < q2.date); DROP INDEX IF EXISTS gun_data_idx_deduplication_query;

-- Remove rows with a state field not equal to MA
DELETE FROM gun_data WHERE state != "MA" AND state != "";

-- Add OG_MAKE column to gun_data for make_updates.sql to do its magic on
ALTER TABLE gun_data ADD COLUMN OG_MAKE TEXT;

-- Correct incorrectly spelled fields
.read make_updates.sql

-- Output back into csv
.headers on
.output gun_data.csv
SELECT * FROM gun_data;
.output stdout
EOF
