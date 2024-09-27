# Notes
Connect to virtual PC (only from my IP at the moment)
To give access to another IP address for SSH
+ In security group sg-0ac5bad340083ed27 - ec2-rds-1 add a new rule with Type SSH and source 80.233.0.0/16 for Three network
+  ssh -i .\RangeTherapyKeyPaid.pem ec2-user@3.250.121.122
pem file is in lastpass

Connect from virtual PC to database - or use PGAdmin
+  psql --host database-mqtt.cxmymmyeyegb.eu-west-1.rds.amazonaws.com --port=5432 --dbname=postgres --username=postgres
Password is in lastpass

Script to generate a table with 200 columns
DO $$
DECLARE
    col_num INT;
    sql_text TEXT := 'ALTER TABLE cell_voltages ';
BEGIN
    FOR col_num IN 1..200 LOOP
        sql_text := sql_text || 'ADD COLUMN cell' || col_num || ' real';
        IF col_num < 200 THEN
            sql_text := sql_text || ', ';
        END IF;
    END LOOP;
    sql_text := sql_text || ';';
    EXECUTE sql_text;
END $$;

Script to push data into these columns
SELECT
    'INSERT INTO cell_voltages (' ||
    string_agg(column_name, ', ') || ') ' || 
    'SELECT ' || string_agg('elem->>\'' || column_name || '\' AS ' || column_name, ', ') ||
    ' FROM json_data, LATERAL jsonb_array_elements(data) as elem;'
FROM information_schema.columns
WHERE table_name = 'cell_voltages';

DO $$
DECLARE
    col_num INT;
    insert_sql TEXT := 'INSERT INTO voltages_table (';
    select_sql TEXT := 'SELECT ';
BEGIN
    -- Dynamically generate the INSERT statement for 400 columns
    FOR col_num IN 1..11 LOOP
        insert_sql := insert_sql || 'cell' || col_num;
        select_sql := select_sql || '(jsonb_array_elements_text(data->''voltages'')::numeric)[' || col_num || '] AS cell' || col_num;
        
        -- Add comma separators, except after the last column
        IF col_num < 11 THEN
            insert_sql := insert_sql || ', ';
            select_sql := select_sql || ', ';
        END IF;
    END LOOP;
    
    -- Close the INSERT statement
    insert_sql := insert_sql || ') ';
    
    -- Close the SELECT statement
    select_sql := select_sql || ' FROM json_data;';
    
    -- Execute the dynamically generated SQL statement
    EXECUTE insert_sql || select_sql;
END $$;


BEGIN
insert into cell_voltages (timestamp,client_id, cell1, cell2,cell3,cell4,cell5,cell6,cell7,cell8,cell9,cell10,cell11,cell12,cell13,cell14,cell15,cell16,cell17,cell18,cell19,cell20,cell21,cell22,cell23,cell24,cell25,cell26,cell27,cell28,cell29,cell30,cell31,cell32,cell33,cell34,cell35,cell36,cell37,cell38,cell39,cell40,cell41,cell42,cell43,cell44,cell45,cell46,cell47,cell48,cell49,cell50,cell51,cell52,cell53,cell54,cell55,cell56,cell57,cell58,cell59,cell60,cell61,cell62,cell63,cell64,cell65,cell66,cell67,cell68,cell69,cell70,cell71,cell72,cell73,cell74,cell75,cell76,cell77,cell78,cell79,cell80,cell81,cell82,cell83,cell84,cell85,cell86,cell87,cell88,cell89,cell90,cell91,cell92,cell93,cell94,cell95,cell96,cell97,cell98,cell99,cell100,cell101,cell102,cell103,cell104,cell105,cell106,cell107,cell108,cell109,cell110,cell111,cell112,cell113,cell114,cell115,cell116,cell117,cell118,cell119,cell120,cell121,cell122,cell123,cell124,cell125,cell126,cell127,cell128,cell129,cell130,cell131,cell132,cell133,cell134,cell135,cell136,cell137,cell138,cell139,cell140,cell141,cell142,cell143,cell144,cell145,cell146,cell147,cell148,cell149,cell150,cell151,cell152,cell153,cell154,cell155,cell156,cell157,cell158,cell159,cell160,cell161,cell162,cell163,cell164,cell165,cell166,cell167,cell168,cell169,cell170,cell171,cell172,cell173,cell174,cell175,cell176,cell177,cell178,cell179,cell180,cell181,cell182,cell183,cell184,cell185,cell186,cell187,cell188,cell189,cell190,cell191,cell192,cell193,cell194,cell195,cell196,cell197,cell198,cell199,cell200)
select new.arrived, new.client_id, 
	new.voltages[1] AS cell1, 
	(jsonb_array_elements_text(new.payload->'cell_voltages')::numeric)[2] AS cell2, 
	(jsonb_array_elements_text(new.payload->'cell_voltages')::numeric)[3] AS cell3, 
	(jsonb_array_elements_text(new.payload->'cell_voltages')::numeric)[4] AS cell4, 
	(jsonb_array_elements_text(new.payload->'cell_voltages')::numeric)[5] AS cell5, 
	(jsonb_array_elements_text(new.payload->'cell_voltages')::numeric)[6] AS cell6, 
	(jsonb_array_elements_text(new.payload->'cell_voltages')::numeric)[7] AS cell7, 
	(jsonb_array_elements_text(new.payload->'cell_voltages')::numeric)[8] AS cell8, 
	(jsonb_array_elements_text(new.payload->'cell_voltages')::numeric)[9] AS cell9, 
	(jsonb_array_elements_text(new.payload->'cell_voltages')::numeric)[10] AS cell10 
	;
return new;
END;

+ generate code to insert the cell voltages
DO $$
DECLARE
    col_num INT;
    select_sql TEXT := ' ';
BEGIN
    -- Dynamically generate the INSERT statement for 400 columns
    FOR col_num IN 1..200 LOOP
        select_sql := select_sql || 'new.voltages[' || col_num || '] AS cell' || col_num || '\n';
        
        -- Add comma separators, except after the last column
        IF col_num < 200 THEN
            select_sql := select_sql || ', ';
        END IF;
    END LOOP;
    
    
    -- Execute the dynamically generated SQL statement
    EXECUTE select_sql;
END $$;