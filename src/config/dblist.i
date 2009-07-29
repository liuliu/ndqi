/* name, type, mode, locaction1, location2 */
NQ_DATABASE("admin", NQTTCTDB, 0, "admin.db", NULL)
NQ_DATABASE("autotag", NQTTDB, 0, "autotag.db", NULL)
NQ_DATABASE("exif", NQTTCTDB, 0, "exif.db", "make|s model|s year|n month|n day|n hour|n minute|n second|n weekday|n datetime|n exposure|n fnumber|n focal|n flash|n iso|n")
NQ_DATABASE("gist", NQTFDB, 0, "gist.db", NULL)
NQ_DATABASE("lfd", NQTBWDB, NQBW_LIKE_BEST_MATCH_COUNT, "lfd.db", NULL)
NQ_DATABASE("lh", NQTFDB, 0, "lh.db", NULL)
NQ_DATABASE("tag", NQTTDB, 0, "tag.db", NULL)
