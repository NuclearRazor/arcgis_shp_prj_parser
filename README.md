**ArcGIS prj parser**

ArcGIS prj (non wkt, projection config data) file parser

- all numeric representations in section values are parsed as double
- the config line must be without spaces, otherwise some values may be considered incorrect, examples of the .shp file config lines to parse:

"GEOGCS[\"GCS_WGS_1984\",PRIMEM[\"Greenwich\",0.0]]";
"GEOGCS[\"GCS_WGS_1984\",UNIT[\"T\",10.0],PARAMETER[\"False_Easting\",500000.0]]";
"GEOGCS[\"GCS_WGS_1984\",DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137.000000,298.257224]],PRIMEM[\"Greenwich\",0.0],UNIT[\"Kilometer\",1000.0]]";
"GEOGCS[\"GCS_Pulkovo_1942\",DATUM[\"D_Pulkovo_1942\",SPHEROID[\"Krasovsky_1940\",6378245.0,298.3]],PRIMEM[\"Greenwich\",0.0],UNIT[\"Degree\",0.0174532925199433,,666.0010098,1.0]]";
"GEOGCS[\"GCS_WGS_1984\",UNIT1[\"Kilometer\",UNIT2[\"R\",UNIT3[\"Kilometer\",10.0,,-23.0,45,-90,,,90]]]]";
"GEOGCS[\"GCS_WGS_1984\",UNIT1[\"Kilometer\",UNIT2[\"Rr\",UNIT3[\"Kilometer\",10.0,,-23.0,45,-90]]],DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137.000000,298.257224]],UNIT[\"Kilometer\",1000.0],PARAMETER2[\"Central_Meridian_Test\",-124.5],PROJECTION[\"Lambert_Conformal_Conic\"],PRIMEM[\"Greenwich\",0.0],PROJECTION2[\"Lambert_Conformal_Conic_Test\"],PROJECTION[\"Lambert_Conformal_Conic\"]]";
"GEOGCS[\"GCS_WGS_1984\",PROJECTION[\"Lambert_Conformal_Conic\"],PRIMEM[\"Greenwich2\",0.0]]";

