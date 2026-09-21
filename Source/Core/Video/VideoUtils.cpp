#include "VideoUtils.h"

namespace VideoUtils {
    std::set<std::string> videoFilesExtensions = {
        "asf", "avi", "mpeg", "mpg", "mp2", "divx", "vob", "flv", "wmv", "mkv",
        "mp4", "ts", "mov", "mpeg2ts", "3gp", "mpeg1", "mpeg2", "mpeg4", "mv4",
        "rmvb", "qt", "hdmov", "m4v", "ogv", "m2v", "webm", "wmp", "wm", "mpe",
        "m1v", "mpv2", "mp2v", "tp", "tpr", "trp", "ifo", "ogm", "m4p", "m4b",
        "3gpp", "3g2", "3gp2", "rm", "ram", "rpm", "nsv", "dpg", "m2ts", "m2t",
        "mts", "dvr-ms", "k3g", "skm", "evo", "nsr", "amv", "wtv", "f4v", "mxf"
    };
    std::set<std::string> audioFilesExtensions = {
        "aa", "aac", "ac3", "adx", "ahx", "aiff", "ape", "asf", "asx", "au",
        "snd", "aud", "dmf", "dts", "dxd", "flac", "la", "m4a", "mmf", "mod",
        "mp1", "mp2", "mp3", "mp4", "mpc", "ofr", "oga", "ogg", "opus", "pac",
        "pxd", "ra", "rka", "sb0", "shn", "tta", "voc", "vqf", "wav", "wma",
        "wv", "xm", "cd", "mqa", "mid", "mpa", "m1a", "m2a", "mka", "eac3",
        "dtshd", "tak", "cda", "dsf", "aif", "amr"
    };
}