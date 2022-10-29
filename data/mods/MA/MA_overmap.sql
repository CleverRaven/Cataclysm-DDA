--MA_overmap.sqlite3

select distinct
	--om_pagenumber,
	om_y, om_x,
	--omt_pagenumber,
	omt_y, omt_x,
	--land_use_code,
(case when land_use_code is null then 9 --null - must be Ocean (or some other state)
when land_use_code =  1  then 0 --{ $overmap_terrain="field"} #Cropland
when land_use_code =  2  then 0 --{ $overmap_terrain="field"} #Pasture
when land_use_code =  3  then 2 --{ $overmap_terrain="forest_thick"} #Forest
when land_use_code =  4  then 3 --{ $overmap_terrain="forest_water"} #Non-Forested Wetland
when land_use_code =  5  then 0 --{ $overmap_terrain="field"} #Mining
when land_use_code =  6  then 0 --{ $overmap_terrain="field"} #Open Land
when land_use_code =  7  then 0 --{ $overmap_terrain="field"} #Participation Recreation
when land_use_code =  8  then 0 --{ $overmap_terrain="field"} #Spectator Recreation
when land_use_code =  9  then 4 --{ $overmap_terrain="lake_surface"} #Water-Based Recreation
when land_use_code = 10  then 0 --{ $overmap_terrain="field"} #Multi-Family Residential
when land_use_code = 11  then 0 --{ $overmap_terrain="field"} #High Density Residential
when land_use_code = 12  then 0 --{ $overmap_terrain="field"} #Medium Density Residential
when land_use_code = 13  then 0 --{ $overmap_terrain="field"} #Low Density Residential
when land_use_code = 14  then 3 --{ $overmap_terrain="forest_water"} #Saltwater Wetland
when land_use_code = 15  then 0 --{ $overmap_terrain="field"} #Commercial
when land_use_code = 16  then 0 --{ $overmap_terrain="field"} #Industrial
when land_use_code = 17  then 0 --{ $overmap_terrain="field"} #Transitional
when land_use_code = 18  then 0 --{ $overmap_terrain="field"} #Transportation
when land_use_code = 19  then 0 --{ $overmap_terrain="field"} #Waste Disposal
when land_use_code = 20  then 4 --{ $overmap_terrain="lake_surface"} #Water
when land_use_code = 23  then 3 --{ $overmap_terrain="forest_water"} #Cranberry bog
when land_use_code = 24  then 0 --{ $overmap_terrain="field"} #Powerline/Utility
when land_use_code = 25  then 5 --{ $overmap_terrain="lake_shore"} #Saltwater Sandy Beach
when land_use_code = 26  then 0 --{ $overmap_terrain="field"} #Golf Course
when land_use_code = 29  then 4 --{ $overmap_terrain="lake_surface"} #Marina
when land_use_code = 31  then 0 --{ $overmap_terrain="field"} #Urban Public/Institutional
when land_use_code = 34  then 0 --{ $overmap_terrain="field"} #Cemetery
when land_use_code = 35  then 0 --{ $overmap_terrain="field"} #Orchard
when land_use_code = 36  then 1 --{ $overmap_terrain="forest"} #Nursery
when land_use_code = 37  then 3 --{ $overmap_terrain="forest_water"} #Forested Wetland
when land_use_code = 38  then 0 --{ $overmap_terrain="field"} #Very Low Density Residential
when land_use_code = 39  then 0 --{ $overmap_terrain="field"} #Junkyard
when land_use_code = 40  then 1 --{ $overmap_terrain="forest"} #Brushland/Successional
else 0 end --any other
) as "ot"
from omt
--where
--	om_y = 21 and om_x = 65
order by
	om_pagenumber, om_y, om_x, omt_pagenumber, omt_y, omt_x
;

--0   field
--1   forest
--2   forest_thick
--3   forest_water
--4   lake_surface
--5   lake_shore
--999 lake_surface - represents ocean/some other state
-- empty_rock

/*
select distinct
	omt.om_pagenumber, omt.om_y, omt.om_x,
	coord.omt_pagenumber, coord.omt_y, coord.omt_x
from
	coord
	left join omt
		on (coord.omt_pagenumber = omt.omt_pagenumber)
where
	om_y = 17 and om_x = 46
order by
	omt.om_pagenumber, omt.om_y, omt.om_x,
	coord.omt_pagenumber, coord.omt_y, coord.omt_x
;
*/

/*

#select
#om_y, om_x, count(*)
#from omt
#group by om_y, om_x
#having count(*) <> 180*180
#order by om_y, om_x

#om_y	om_x	count(*)
#2	48	32401
#4	45	32401
#5	42	32401
#6	26	32401
#6	45	32402
#12	39	32402
#12	47	32401
#13	45	32404
#14	37	32401
#14	40	32411
#14	41	32402
#14	44	32402
#15	46	32402
#16	34	32401
#16	45	32403
#16	46	32401
#17	32	32401
#18	2	32403
#18	18	32401
#19	15	32401
#19	39	32402
#21	2	32401
#23	52	32405
#23	53	32401
#25	42	32401
#28	41	32405
#28	51	32401
#28	56	32401
#29	48	32403
#32	49	32402
#32	51	32402
#36	46	32404
*/