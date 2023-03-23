#include "WAV.h"
#include "fat32.h"

void write_wav(fat32_fs_t* fs, pi_dirent_t* root, int16_t* buf, char* filename, int sample_rate) {

    fat32_delete(fs, root, filename);
    assert(fat32_create(fs, root, filename, 0));

    INIT_WAV_HEADER(w);
    w.bits_per_sample = 16;
    w.sample_rate = sample_rate;
    w.byterate = w.sample_rate * w.channels * w.bits_per_sample / 16;
    w.block_align = w.channels * w.bits_per_sample / 16;
    w.overall_size = num_bytes - 8;
    w.data_size = (N * w.channels * w.bits_per_sample) / 16;

    memcpy(buf, &w, sizeof(wav_header_t));

    pi_file_t test = (pi_file_t) {
        .data = (char *)buf,
        .n_data = num_bytes,
        .n_alloc = num_bytes,
    };

    assert(fat32_write(&fs, &root, test_name, &test));
    pi_file_t *read_file_after = fat32_read(&fs, &root, test_name);
    assert(test.n_data == read_file_after->n_data);
    for (int i = 0; i < read_file_after->n_data; i++) {
        assert(read_file_after->data[i] == test.data[i]);
    }
    printk("PASS: %s\n", __FILE__);

}