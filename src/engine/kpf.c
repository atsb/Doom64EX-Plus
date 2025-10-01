#include "kpf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <zlib.h>

// atsb: reading a KPF file with zlib..  not too ugly..

// these two are for 'doing the same thing multiple times'.  Makes sense in this case
static unsigned short rd16(const unsigned char* p) {
    return (unsigned short)(p[0] | (p[1] << 8));
}
static unsigned int rd32(const unsigned char* p) {
    return (unsigned int)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

static int str_ieq(const char* a, const char* b) {
    if (!a || !b) 
        return 0;
    while (*a && *b) {
        unsigned char ca = (unsigned char)*a++;
        unsigned char cb = (unsigned char)*b++;
        if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb - 'A' + 'a');
        if (ca != cb) 
            return 0;
    }
    return *a == 0 && *b == 0;
}

// atsb: grabs a buffer and resizes it.  This also fixes a previous bug where I was discarding information
static int KPF_InflateBuffer(FILE* f, long data_off, unsigned int csize,
    unsigned char* out, unsigned int usize)
{
    if (fseek(f, data_off, SEEK_SET) != 0) return 0;

    z_stream z; memset(&z, 0, sizeof(z));
    if (inflateInit2(&z, -MAX_WBITS) != Z_OK) return 0;

    const size_t CHUNK = 64 * 1024;
    unsigned char* inbuf = (unsigned char*)malloc(CHUNK);
    if (!inbuf) { inflateEnd(&z); return 0; }

    z.next_out = out;
    z.avail_out = usize;

    unsigned int remaining = csize;
    int ret = Z_OK;

    while (ret != Z_STREAM_END && z.avail_out > 0) {
        if (z.avail_in == 0) {
            if (remaining == 0) { ret = Z_DATA_ERROR; break; }
            size_t want = remaining < CHUNK ? remaining : CHUNK;
            size_t got = fread(inbuf, 1, want, f);
            if (got == 0) { ret = Z_DATA_ERROR; break; }
            remaining -= (unsigned int)got;
            z.next_in = inbuf;
            z.avail_in = (unsigned int)got;
        }

        ret = inflate(&z, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END && ret != Z_BUF_ERROR) break;
    }

    int ok = (ret == Z_STREAM_END && z.avail_out == 0);

    inflateEnd(&z);
    free(inbuf);
    return ok;
}

static int KPF_ExtractInternalData(const char* kpf_path,
                            const char* inner_path_utf8,
                            unsigned char** out_data,
                            int* out_size,
                            unsigned int max_uncompressed)
{
    *out_data = NULL;
    *out_size = 0;

    FILE* f = fopen(kpf_path, "rb");
    if (!f) return 0;

    if (fseek(f, 0, SEEK_END) != 0) { 
        fclose(f); 
        return 0; 
    }

    long flen = ftell(f);
    long scan_from = flen - 22; 
    if (scan_from < 0) 
        scan_from = 0;
    long min_scan = flen - (22 + 0x10000); 
    if (min_scan < 0) 
        min_scan = 0;

    unsigned int cdir_off = 0;
    for (long pos = scan_from; pos >= min_scan; --pos) {
        if (fseek(f, pos, SEEK_SET) != 0) 
            break;
        unsigned char buf[4];
        if (fread(buf, 1, 4, f) != 4)
            break;
        unsigned int sig = rd32(buf);
        if (sig == ZIP_SIG_EOCD) {
            unsigned char eocd[22];
            if (fseek(f, pos, SEEK_SET) != 0) 
                break;
            if (fread(eocd, 1, 22, f) != 22) 
                break;
            cdir_off = rd32(eocd + 16);
            break;
        }
    }
    if (!cdir_off) { 
        fclose(f); 
        return 0; 
    }

    if (fseek(f, cdir_off, SEEK_SET) != 0) { 
        fclose(f); 
        return 0; 
    }

    for (;;) {
        unsigned char hdr[46];
        if (fread(hdr, 1, 4, f) != 4) 
            break;
        unsigned int sig = rd32(hdr);
        if (sig != ZIP_SIG_CDH) break;

        if (fread(hdr + 4, 1, 42, f) != 42) 
            break;

        // took some basic example code here and shoved it
        unsigned short gpflag = rd16(hdr + 6);
        unsigned short method = rd16(hdr + 10);
        unsigned int   csize  = rd32(hdr + 20);
        unsigned int   usize  = rd32(hdr + 24);
        unsigned short fnlen  = rd16(hdr + 28);
        unsigned short extralen = rd16(hdr + 30);
        unsigned short comlen = rd16(hdr + 32);
        unsigned int   lhofs  = rd32(hdr + 42);

        char* name = (char*)malloc(fnlen + 1);
        if (!name) { 
            fclose(f); 
            return 0; 
        }
        if (fread(name, 1, fnlen, f) != fnlen) { 
            free(name); 
            fclose(f); 
            return 0; 
        }
        name[fnlen] = 0;

        if (fseek(f, extralen + comlen, SEEK_CUR) != 0) { 
            free(name); 
            fclose(f); 
            return 0; 
        }

        if (!(gpflag & 1) && str_ieq(name, inner_path_utf8)) {
            if (usize > max_uncompressed) { 
                free(name); 
                fclose(f); 
                return 0; 
            }

            if (fseek(f, lhofs, SEEK_SET) != 0) { 
                free(name); 
                fclose(f); 
                return 0; 
            }
            unsigned char lfh[30];
            if (fread(lfh, 1, 30, f) != 30) { 
                free(name); 
                fclose(f); 
                return 0; 
            }
            if (rd32(lfh) != ZIP_SIG_LFH) { 
                free(name); 
                fclose(f); 
                return 0; 
            }
            unsigned short lfn = rd16(lfh + 26);
            unsigned short lextra = rd16(lfh + 28);
            long data_off = lhofs + 30 + lfn + lextra;

            unsigned char* out = (unsigned char*)malloc(usize);
            if (!out) { 
                free(name); 
                fclose(f);
                return 0; 
            }

            int ok = 0;
            if (method == 0) {
                if (fseek(f, data_off, SEEK_SET) == 0 &&
                    fread(out, 1, usize, f) == usize) ok = 1;
            } else if (method == 8) {
                ok = KPF_InflateBuffer(f, data_off, csize, out, usize);
            }

            if (ok) {
                *out_data = out;
                *out_size = (int)usize;
                free(name);
                fclose(f);
                return 1;
            }
            free(out);
        }
        free(name);
    }
    fclose(f);
    return 0;
}

int KPF_ExtractFile(const char* kpf_path,
                     const char* inner_path_utf8,
                     unsigned char** out_data,
                     int* out_size)
{
    return KPF_ExtractInternalData(kpf_path, inner_path_utf8, out_data, out_size, UINT_MAX);
}

int KPF_ExtractFileCapped(const char* kpf_path,
                            const char* inner_path_utf8,
                            unsigned char** out_data,
                            int* out_size,
                            int max_uncompressed)
{
    if (max_uncompressed < 0) max_uncompressed = 0;
    return KPF_ExtractInternalData(kpf_path, inner_path_utf8, out_data, out_size,
                            (unsigned int)max_uncompressed);
}
