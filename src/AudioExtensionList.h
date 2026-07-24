#pragma once

#include <QStringList>

namespace AudioExtensionList {
    inline QStringList getDefaultExtensions() {
        return QStringList{
            "302", "3gp", "4gv", "4xm", "8svx", "aa3", "aac", "ac", "ac3", "ac4", "adx", "afc", "agm", "aica", "aiff", "alac", "alp", "als", "amr", "amv", "apac", "apc", "ape", "apm", "argo", "at3", "at9", "au", "aud", "avi", "awb", "bik", "bmv", "bonk", "c2", "caf", "cbd", "celt", "cnv", "dat", "daud", "dff", "dfpwm", "dpc", "dsf", "dss", "dst", "dsv", "dtk", "dts", "dtshd", "dv", "dvd", "ea", "eac3", "eacs", "ec3", "evc", "flac", "flv", "fst", "ftr", "g722", "g723", "g726", "g729", "gdv", "gsm", "hca", "hcom", "iac", "imc", "iss", "latm", "lbc", "loas", "lxf", "m2ts", "m4a", "mhm1", "mjpg", "mlp", "moflex", "mov", "mp1", "mp2", "mp3", "mp4", "mpa", "mpc", "mpeg", "msp", "mtaf", "mtf", "mts", "mve", "mxf", "oga", "ogg", "oki", "oma", "omg", "opus", "osq", "paf", "pcm", "qcp", "qoa", "qt", "ra", "rad", "ralf", "raw", "rka", "rm", "roq", "sdx2", "sga", "shn", "smjpeg", "smk", "smv", "snd", "sol", "sonic", "spx", "ssi", "str", "sud", "swf", "tak", "thd", "thp", "ts", "tta", "vag", "vima", "vmd", "vob", "voc", "vqf", "wad", "wav", "wavarc", "webm", "wma", "wv", "wvc", "xa", "xas", "xma", "xmd"
        };
    }
}