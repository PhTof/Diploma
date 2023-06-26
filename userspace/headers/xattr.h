static struct xflags {
        uint    flag;
        char    *shortname;
        char    *longname;
} xflags[] = {
        { FS_XFLAG_REALTIME,            "r", "realtime"         },
        { FS_XFLAG_PREALLOC,            "p", "prealloc"         },
        { FS_XFLAG_IMMUTABLE,           "i", "immutable"        },
        { FS_XFLAG_APPEND,              "a", "append-only"      },
        { FS_XFLAG_SYNC,                "s", "sync"             },
        { FS_XFLAG_NOATIME,             "A", "no-atime"         },
        { FS_XFLAG_NODUMP,              "d", "no-dump"          },
        { FS_XFLAG_RTINHERIT,           "t", "rt-inherit"       },
        { FS_XFLAG_PROJINHERIT,         "P", "proj-inherit"     },
        { FS_XFLAG_NOSYMLINKS,          "n", "nosymlinks"       },
        { FS_XFLAG_EXTSIZE,             "e", "extsize"          },
        { FS_XFLAG_EXTSZINHERIT,        "E", "extsz-inherit"    },
        { FS_XFLAG_NODEFRAG,            "f", "no-defrag"        },
        { FS_XFLAG_FILESTREAM,          "S", "filestream"       },
        { FS_XFLAG_DAX,                 "x", "dax"              },
        { FS_XFLAG_COWEXTSIZE,          "C", "cowextsize"       },
        { FS_XFLAG_HASATTR,             "X", "has-xattr"        },
        { 0, NULL, NULL }
};

void
printxattr(
        uint            flags,
        int             verbose,
        int             dofname,
        const char      *fname,
        int             dobraces,
        int             doeol)
{
        struct xflags   *p;
        int             first = 1;

        if (dobraces)
                fputs("[", stdout);
        for (p = xflags; p->flag; p++) {
                if (flags & p->flag) {
                        if (verbose) {
                                if (first)
                                        first = 0;
                                else
                                        fputs(", ", stdout);
                                fputs(p->longname, stdout);
                        } else {
                                fputs(p->shortname, stdout);
                        }
                } else if (!verbose) {
                        fputs("-", stdout);
                }
        }
        if (dobraces)
                fputs("]", stdout);
        if (dofname)
                printf(" %s ", fname);
        if (doeol)
                fputs("\n", stdout);
}

