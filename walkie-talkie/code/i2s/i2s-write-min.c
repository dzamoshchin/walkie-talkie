#include "rpi.h"
#include "i2s.h"
#include "../fat32/fat32.h"
#include "wav.h"

#define SECS 5
#define SAMPLE_RATE 44100
#define N (SAMPLE_RATE * SECS)

void notmain(void) {
    size_t num_bytes = sizeof(wav_header_t) + N * sizeof(uint16_t);
    int16_t *buf = (int16_t *)kmalloc(num_bytes);

    i2s_init(16, SAMPLE_RATE);
    i2s_enable_rx();

    unsigned start = timer_get_usec();
    int offset = sizeof(wav_header_t);
    for (int i = 0; i < N; i++) {
        buf[i + offset] = ((i2s_read_sample() & 0xFFFF));
    }
    unsigned end = timer_get_usec();

    uart_init();
    kmalloc_init();
    pi_sd_init();

    printk("Reading the MBR.\n");
    mbr_t *mbr = mbr_read();

    printk("Loading the first partition.\n");
    mbr_partition_ent_t partition;
    memcpy(&partition, mbr->part_tab1, sizeof(mbr_partition_ent_t));
    assert(mbr_part_is_fat32(partition.part_type));

    printk("Loading the FAT.\n");
    fat32_fs_t fs = fat32_mk(&partition);

    printk("Loading the root directory.\n");
    pi_dirent_t root = fat32_get_root(&fs);

    printk("Creating test.wav\n");
    char *test_name = "MSG.WAV";
    fat32_delete(&fs, &root, test_name);
    assert(fat32_create(&fs, &root, test_name, 0));

    INIT_WAV_HEADER(w);
    w.bits_per_sample = 16;
    w.sample_rate = SAMPLE_RATE;
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
    // char *te = (char *)buf;
    // for (int i = 0; i < 100; i++) {
    //     printk("%x\n", *(te + i));
    // }
    assert(fat32_write(&fs, &root, test_name, &test));
    pi_file_t *read_file_after = fat32_read(&fs, &root, test_name);
    assert(test.n_data == read_file_after->n_data);
    for (int i = 0; i < read_file_after->n_data; i++) {
        //printk("%d\n", i);
        assert(read_file_after->data[i] == test.data[i]);
    }
    printk("passed quivalence\n");
    printk("Check your SD card for a file called 'MSG.WAV'\n");

    printk("PASS: %s\n", __FILE__);
    clean_reboot();
}