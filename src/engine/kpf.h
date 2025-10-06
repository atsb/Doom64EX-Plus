#define ZIP_SIG_EOCD   0x06054b50u
#define ZIP_SIG_CDH    0x02014b50u
#define ZIP_SIG_LFH    0x04034b50u

int KPF_ExtractFile(const char* kpf_path,
        const char* inner_path_utf8,
        unsigned char** out_data,
        int* out_size);

int KPF_ExtractFileCapped(const char* kpf_path,
        const char* inner_path_utf8,
        unsigned char** out_data,
        int* out_size,
        int max_uncompressed);
