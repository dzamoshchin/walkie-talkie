#include "rpi.h"
#include "i2s.h"
#include "../fat32/fat32.h"
#include "wav.h"

#define SECS 5
#define SAMPLE_RATE 44100
#define N (SAMPLE_RATE * SECS)
#define DOUBLE_TAP_THRESHOLD 400000

int32_t big_to_little_endian(int32_t num)
{
    int32_t swapped = ((num>>24)&0xff) | // move byte 3 to byte 0
                        ((num<<8)&0xff0000) | // move byte 1 to byte 2
                        ((num>>8)&0xff00) | // move byte 2 to byte 1
                        ((num<<24)&0xff000000); // byte 0 to byte 3
    return swapped;
}

void notmain(void) {
    size_t num_bytes = sizeof(wav_header_t) + N * sizeof(int32_t);
    int32_t *buf = (int32_t *)kmalloc(num_bytes);

    i2s_init();
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

    // unsigned start = timer_get_usec();
    // int offset = sizeof(wav_header_t) / sizeof(int32_t);
    // for (int i = 0; i < N; i++) {
    //     buf[i + offset] = i2s_read_sample();
    // }
    // unsigned end = timer_get_usec();

    //old endian code that is useless
    // for (int i = 0; i < N; i++){
    //     buf[i + offset] = big_to_little_endian(buf[i + offset]);
    // }

    const int button = 27;
    const int offset = sizeof(wav_header_t) / sizeof(int32_t);

    gpio_set_input(button);

    unsigned last_tap_time = 0;
    int loc = 0;
    int file_num = 0;

    printk("Creating new sound file!\n"); 

    char *test_name = "ARN .WAV";
    test_name[3] = 48 + file_num;
    printk("Sound file is called ");
    printk(test_name);
    printk("\n");

    fat32_delete(&fs, &root, test_name);
    assert(fat32_create(&fs, &root, test_name, 0));

    INIT_WAV_HEADER(w);
    w.overall_size = num_bytes - 8;
    w.data_size = (N * w.channels * w.bits_per_sample) / 8;

    memcpy(buf, &w, sizeof(wav_header_t));
    printk("setup done\n");
    while (1) {
        if (gpio_read(button)) {
            printk("read\n");
            unsigned start = timer_get_usec();
            while(1) {
                buf[loc + offset] = i2s_read_sample();
                printk("read sample\n");
                loc++;
                if (!gpio_read(button)) {
                    printk("not read\n");
                    break;
                }
            }
            unsigned end = timer_get_usec();

            // this is how long the tap was
            printk("%d\n", end-start);


            if (end - last_tap_time < DOUBLE_TAP_THRESHOLD) {
                printk("double tap!\n");
                printk("Creating new sound file!\n");
                file_num++;

                test_name = "ARN .WAV";
                test_name[3] = 48 + file_num;
                printk("Sound file is called ");
                printk(test_name);
                printk("\n");

                fat32_delete(&fs, &root, test_name);
                assert(fat32_create(&fs, &root, test_name, 0));

                memcpy(buf, &w, sizeof(wav_header_t));
                loc = 0;
            } else {
                uint32_t num_bytes = (SAMPLE_RATE * (end - start))/1000000;
                pi_file_t test = (pi_file_t) {
                    .data = (char *)buf,
                    .n_data = num_bytes,
                    .n_alloc = num_bytes,
                };
                fat32_extend(&fs, &root, test_name, &test);
            }

            last_tap_time = timer_get_usec();
        }
    }



    // char *te = (char *)buf;
    // for (int i = 0; i < 100; i++) {
    //     printk("%x\n", *(te + i));
    // }
    // assert(fat32_write(&fs, &root, test_name, &test));
    // pi_file_t *read_file_after = fat32_read(&fs, &root, test_name);
    // assert(test.n_data == read_file_after->n_data);
    // for (int i = 0; i < read_file_after->n_data; i++) {
    //     //printk("%d\n", i);
    //     assert(read_file_after->data[i] == test.data[i]);
    // }
    // printk("passed quivalence\n");
    // printk("Check your SD card for a file called 'TEST.WAV'\n");

    printk("PASS: %s\n", __FILE__);
    clean_reboot();
}
