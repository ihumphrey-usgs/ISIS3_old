APPNAME = gllssi2isis

labels.txt.IGNORELINES = Bytes StartByte ByteOrder TileSamples TileLines

include $(ISISROOT)/make/isismake.tsts

commands:
	 $(APPNAME) from=$(INPUT)/0826r.lbl \
	  to=$(OUTPUT)/gllssi2isisTruth1.cub > /dev/null;
	  catlab from=$(OUTPUT)/gllssi2isisTruth1.cub to=$(OUTPUT)/labels.txt > /dev/null;
