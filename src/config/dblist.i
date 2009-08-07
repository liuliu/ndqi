/* name, type, mode, locaction, extra config */
NQ_DATABASE("admin", NQTTCTDB, 0, "admin.db", NULL)
NQ_DATABASE("exif", NQTTCTDB, 0, "exif.db", "make|s model|s year|n month|n day|n hour|n minute|n second|n weekday|n datetime|n exposure|n fnumber|n focal|n flash|n iso|n")
NQ_DATABASE("gist", NQTFDB, 0, "gist.db", NULL)
NQ_DATABASE("kod", NQTTCTDB, 0, "kod.db", "face|n people|n")
NQ_DATABASE("lfd", NQTBWDB, NQBW_LIKE_BEST_MATCH_COUNT, "lfd.db", NULL)
NQ_DATABASE("lh", NQTFDB, 0, "lh.db", NULL)
NQ_DATABASE("ocr", NQTTDB, 0, "ocr.db", NULL)
NQ_DATABASE("tag", NQTTDB, 0, "tag.db", NULL)
