#include "rpi.h"
#include "fat32.h"
#include "fat32-helpers.h"
#include "pi-sd.h"

// Print extra tracing info when this is enabled.  You can and should add your
// own.
static int trace_p = 1; 
static int init_p = 0;

fat32_boot_sec_t boot_sector;


fat32_fs_t fat32_mk(mbr_partition_ent_t *partition) {
  demand(!init_p, "the fat32 module is already in use\n");
  // Read the boot sector (of the partition) off the SD card.
  //paul p. 1
  pi_sd_read(&boot_sector, partition->lba_start, 1);

  // Verify the boot sector (also called the volume id, `fat32_volume_id_check`)
  fat32_volume_id_check(&boot_sector);

  // Read the FS info sector (the sector immediately following the boot
  // sector) and check it (`fat32_fsinfo_check`, `fat32_fsinfo_print`)
  assert(boot_sector.info_sec_num == 1);
  struct fsinfo *cur_info = pi_sec_read(partition->lba_start + boot_sector.info_sec_num, 1);
  fat32_fsinfo_check(cur_info);
  //fat32_volume_id_print("volume id\n", &boot_sector);
  //fat32_fsinfo_print("FS info sector\n", cur_info);

  // END OF PART 2
  // The rest of this is for Part 3:

  // Calculate the fat32_fs_t metadata, which we'll need to return.
  unsigned lba_start = partition->lba_start; // from the partition
  unsigned fat_begin_lba = lba_start + boot_sector.reserved_area_nsec; // the start LBA + the number of reserved sectors
  unsigned cluster_begin_lba = fat_begin_lba + (boot_sector.nfats * boot_sector.nsec_per_fat); // the beginning of the FAT, plus the combined length of all the FATs
  unsigned sec_per_cluster = boot_sector.sec_per_cluster; // from the boot sector
  unsigned root_first_cluster = boot_sector.first_cluster; // from the boot sector
  // bytes per sector = 512 and 4 bytes per entry, so 512/4 = 128
  unsigned n_entries = (boot_sector.nsec_per_fat * 128); // from the boot sector

  /*
   * TODO: Read in the entire fat (one copy: worth reading in the second and
   * comparing).
   *
   * The disk is divided into clusters. The number of sectors per
   * cluster is given in the boot sector byte 13. <sec_per_cluster>
   *
   * The File Allocation Table has one entry per cluster. This entry
   * uses 12, 16 or 28 bits for FAT12, FAT16 and FAT32.
   *
   * Store the FAT in a heap-allocated array.
   */
  uint32_t *fat;
  fat = pi_sec_read(fat_begin_lba, boot_sector.nsec_per_fat);

  // Create the FAT32 FS struct with all the metadata
  fat32_fs_t fs = (fat32_fs_t) {
    .lba_start = lba_start,
      .fat_begin_lba = fat_begin_lba,
      .cluster_begin_lba = cluster_begin_lba,
      .sectors_per_cluster = sec_per_cluster,
      .root_dir_first_cluster = root_first_cluster,
      .fat = fat,
      .n_entries = n_entries,
  };

  if (trace_p) {
    trace("begin lba = %d\n", fs.fat_begin_lba);
    trace("cluster begin lba = %d\n", fs.cluster_begin_lba);
    trace("sectors per cluster = %d\n", fs.sectors_per_cluster);
    trace("root dir first cluster = %d\n", fs.root_dir_first_cluster);
  }

  init_p = 1;
  return fs;
}

// Given cluster_number, get lba.  Helper function.
static uint32_t cluster_to_lba(fat32_fs_t *f, uint32_t cluster_num) {
  printk("cluster number is: %d\n", cluster_num);
  assert(cluster_num >= 2);
  // Calculate LBA from cluster number, cluster_begin_lba, and
  // sectors_per_cluster
  unsigned lba = f->cluster_begin_lba + (f->sectors_per_cluster * (cluster_num - 2));
  if (trace_p) trace("cluster %d to lba: %d\n", cluster_num, lba);
  return lba;
}

pi_dirent_t fat32_get_root(fat32_fs_t *fs) {
  demand(init_p, "fat32 not initialized!");
  // Return the information corresponding to the root directory (just
  // cluster_id, in this case)

  return (pi_dirent_t) {
    .name = "",
      .raw_name = "",
      .cluster_id = fs->root_dir_first_cluster, // fix this
      .is_dir_p = 1,
      .nbytes = 0,
  };
}

// Given the starting cluster index, get the length of the chain.  Helper
// function.
static uint32_t get_cluster_chain_length(fat32_fs_t *fs, uint32_t start_cluster) {
  // Walk the cluster chain in the FAT until you see a cluster where
  // `fat32_fat_entry_type(cluster) == LAST_CLUSTER`.  Count the number of
  // clusters.
  uint32_t counter = 0;
  uint32_t cur_cluster = start_cluster;
  while (fat32_fat_entry_type(cur_cluster) != LAST_CLUSTER) {
    cur_cluster = fs->fat[cur_cluster];
    counter++;
  }
  return counter; 
}

// Given the starting cluster index, read a cluster chain into a contiguous
// buffer.  Assume the provided buffer is large enough for the whole chain.
// Helper function.
static void read_cluster_chain(fat32_fs_t *fs, uint32_t start_cluster, uint8_t *data) {
  // Walk the cluster chain in the FAT until you see a cluster where
  // fat32_fat_entry_type(cluster) == LAST_CLUSTER.  For each cluster, copy it
  // to the buffer (`data`).  Be sure to offset your data pointer by the
  // appropriate amount each time.
  uint32_t cur_cluster = start_cluster;
  while (fat32_fat_entry_type(cur_cluster) != LAST_CLUSTER) {
    pi_sd_read(data, cluster_to_lba(fs, cur_cluster), fs->sectors_per_cluster);
    data += fs->sectors_per_cluster * boot_sector.bytes_per_sec;
    cur_cluster = fs->fat[cur_cluster];
  }
  return;
}

// Converts a fat32 internal dirent into a generic one suitable for use outside
// this driver.
static pi_dirent_t dirent_convert(fat32_dirent_t *d) {
  pi_dirent_t e = {
    .cluster_id = fat32_cluster_id(d),
    .is_dir_p = d->attr == FAT32_DIR,
    .nbytes = d->file_nbytes,
  };
  // can compare this name
  memcpy(e.raw_name, d->filename, sizeof d->filename);
  // for printing.
  fat32_dirent_name(d,e.name);
  return e;
}

// Gets all the dirents of a directory which starts at cluster `cluster_start`.
// Return a heap-allocated array of dirents.
static fat32_dirent_t *get_dirents(fat32_fs_t *fs, uint32_t cluster_start, uint32_t *dir_n) {
  // TODO: figure out the length of the cluster chain (see
  // `get_cluster_chain_length`)

  *dir_n = get_cluster_chain_length(fs, cluster_start);

  // TODO: allocate a buffer large enough to hold the whole directory
  uint32_t *data = kmalloc(*dir_n * boot_sector.bytes_per_sec * boot_sector.sec_per_cluster);

  // TODO: read in the whole directory (see `read_cluster_chain`)
  read_cluster_chain(fs, cluster_start, (uint8_t *) data);
  *dir_n = *dir_n * boot_sector.bytes_per_sec * boot_sector.sec_per_cluster / sizeof(fat32_dirent_t);
  return (fat32_dirent_t *)data;
}

pi_directory_t fat32_readdir(fat32_fs_t *fs, pi_dirent_t *dirent) {
  demand(init_p, "fat32 not initialized!");
  demand(dirent->is_dir_p, "tried to readdir a file!");
  // Use `get_dirents` to read the raw dirent structures from the disk
  uint32_t n_dirents;
  fat32_dirent_t *dirents = get_dirents(fs, dirent->cluster_id, &n_dirents);

  // Allocate space to store the pi_dirent_t return values
  pi_dirent_t *pi_dirents = kmalloc(sizeof(pi_dirent_t) * n_dirents);

  // Iterate over the directory and create pi_dirent_ts for every valid
  // file.  Don't include empty dirents, LFNs, or Volume IDs.  You can use
  // `dirent_convert`.
  uint32_t ndirents_filtered = 0;
  for(int i = 0; i < n_dirents; i++) {
    fat32_dirent_t cur = dirents[i];
    if (!fat32_dirent_free(&cur) && !fat32_dirent_is_lfn(&cur) && !fat32_is_attr(cur.attr, FAT32_VOLUME_LABEL)) {
      pi_dirents[ndirents_filtered] = dirent_convert(&cur);
      ndirents_filtered++;
    }
  }

  // Create a pi_directory_t using the dirents and the number of valid
  // dirents we found
  return (pi_directory_t) {
    .dirents = pi_dirents,
    .ndirents = ndirents_filtered,
  };
}

static int find_dirent_with_name(fat32_dirent_t *dirents, int n, char *filename) {
  // Iterate through the dirents, looking for a file which matches the
  // name; use `fat32_dirent_name` to convert the internal name format to a
  // normal string.
  for (int i = 0; i < n; i++) {
    char name[9];
    fat32_dirent_name(dirents+i, name);
    if (!strcmp(name, filename)) return i;
  }
  return -1;
}

pi_dirent_t *fat32_stat(fat32_fs_t *fs, pi_dirent_t *directory, char *filename) {
  demand(init_p, "fat32 not initialized!");
  demand(directory->is_dir_p, "tried to use a file as a directory");

  // Use `get_dirents` to read the raw dirent structures from the disk
  uint32_t dir_n;
  fat32_dirent_t *dirents = get_dirents(fs, directory->cluster_id, &dir_n);

  // Iterate through the directory's entries and find a dirent with the
  // provided name.  Return NULL if no such dirent exists.  You can use
  // `find_dirent_with_name` if you've implemented it.
  int ret = find_dirent_with_name(dirents, dir_n, filename);
  if(ret == -1) return NULL;

  // Allocate enough space for the dirent, then convert
  // (`dirent_convert`) the fat32 dirent into a Pi dirent.
  pi_dirent_t *dirent = kmalloc(sizeof(pi_dirent_t));
  *dirent = dirent_convert(dirents + ret);
  return dirent;
}

pi_file_t *fat32_read(fat32_fs_t *fs, pi_dirent_t *directory, char *filename) {
  // This should be pretty similar to readdir, but simpler.
  demand(init_p, "fat32 not initialized!");
  demand(directory->is_dir_p, "tried to use a file as a directory!");

  // Read the dirents of the provided directory and look for one matching the provided name
  pi_dirent_t *dirent = fat32_stat(fs, directory, filename);

  // Figure out the length of the cluster chain
  uint32_t chain_length = get_cluster_chain_length(fs, dirent->cluster_id);

  // Allocate a buffer large enough to hold the whole file
  char *data = kmalloc(dirent->nbytes);

  // Read in the whole file (if it's not empty)
  read_cluster_chain(fs, dirent->cluster_id, data);

  // Fill in the pi_file_t
  pi_file_t *file = kmalloc(sizeof(pi_file_t));
  *file = (pi_file_t) {
    .data = data,
    .n_data = dirent->nbytes,
    .n_alloc = chain_length * boot_sector.bytes_per_sec * boot_sector.sec_per_cluster,
  };
  return file;
}

/******************************************************************************
 * Everything below here is for writing to the SD card (Part 7/Extension).  If
 * you're working on read-only code, you don't need any of this.
 ******************************************************************************/

static uint32_t find_free_cluster(fat32_fs_t *fs, uint32_t start_cluster) {
  // Loop through the entries in the FAT until you find a free one
  // (fat32_fat_entry_type == FREE_CLUSTER).  Start from cluster 3.  Panic if
  // there are none left.
  uint32_t init_start_cluster = start_cluster;
  if (start_cluster < 3) start_cluster = 3;
  while (start_cluster < fs->n_entries) {
    if (fat32_fat_entry_type(fs->fat[start_cluster]) == FREE_CLUSTER) {
      return start_cluster;
    }
    start_cluster++;
  }
  if (trace_p) trace("failed to find free cluster from %d\n", start_cluster);
  panic("No more clusters on the disk!\n");
}

static void write_fat_to_disk(fat32_fs_t *fs) {
  // Write the FAT to disk.  In theory we should update every copy of the
  // FAT, but the first one is probably good enough.  A good OS would warn you
  // if the FATs are out of sync, but most OSes just read the first one without
  // complaining.
  if (trace_p) trace("syncing FAT\n");
  //Does this also call cluster chain?
  pi_sd_write(fs->fat, fs->fat_begin_lba, boot_sector.nsec_per_fat);
}

// Given the starting cluster index, write the data in `data` over the
// pre-existing chain, adding new clusters to the end if necessary.
static void write_cluster_chain(fat32_fs_t *fs, uint32_t start_cluster, uint8_t *data, uint32_t nbytes) {
  // Walk the cluster chain in the FAT, writing the in-memory data to the
  // referenced clusters.  If the data is longer than the cluster chain, find
  // new free clusters and add them to the end of the list.
  // Things to consider:
  //  - what if the data is shorter than the cluster chain?
  //  - what if the data is longer than the cluster chain?
  //  - the last cluster needs to be marked LAST_CLUSTER
  //  - when do we want to write the updated FAT to the disk to prevent
  //  corruption?
  //  - what do we do when nbytes is 0?
  //  - what about when we don't have a valid cluster to start with?
  //
  //  This is the main "write" function we'll be using; the other functions
  //  will delegate their writing operations to this.

  if (nbytes == 0) {
    if (trace_p) trace("write called for nbytes = 0!!\n");
    return;
  }
  int start_cluster_type = fat32_fat_entry_type(fs->fat[start_cluster]);
  if (start_cluster_type == BAD_CLUSTER) {
    if (trace_p) trace("write called starting with a bad cluster!\n");
    if (trace_p) trace("%x\n", fs->fat[start_cluster]);
    return;
  }

  // As long as we have bytes left to write and clusters to write them
  // to, walk the cluster chain writing them out.
  uint32_t cur_cluster = start_cluster;
  uint32_t write_inc = fs->sectors_per_cluster * boot_sector.bytes_per_sec;
  uint32_t bytes_written = 0;
  uint32_t prev_cluster = -1;
  while (cur_cluster != LAST_CLUSTER && bytes_written < nbytes) {
    //printk("t\n");
    pi_sd_write(data, cluster_to_lba(fs, cur_cluster), fs->sectors_per_cluster);
    data += write_inc;
    bytes_written += write_inc;
    prev_cluster = cur_cluster;
    cur_cluster = fs->fat[cur_cluster];
  }

  // If we run out of bytes to write before using all the clusters, mark
  // the final cluster as "LAST_CLUSTER" in the FAT, then free all the clusters
  // later in the chain.
  if (bytes_written >= nbytes && cur_cluster != LAST_CLUSTER) {
    fs->fat[prev_cluster] = LAST_CLUSTER;
    uint32_t temp_cluster;
    while (fat32_fat_entry_type(fs->fat[cur_cluster]) != LAST_CLUSTER) {
      temp_cluster = cur_cluster;
      cur_cluster = fs->fat[cur_cluster];
      fs->fat[temp_cluster] = FREE_CLUSTER;
    }
    fs->fat[cur_cluster] = FREE_CLUSTER;
  }

  // If we run out of clusters to write to, find free clusters using the
  // FAT and continue writing the bytes out.  Update the FAT to reflect the new
  // cluster.
  while(bytes_written < nbytes) {
    //printk("q\n");
    cur_cluster = find_free_cluster(fs, 3);
    pi_sd_write(data, cluster_to_lba(fs, cur_cluster), fs->sectors_per_cluster);
    data += write_inc;
    bytes_written += write_inc;
    fs->fat[prev_cluster] = cur_cluster;
    fs->fat[cur_cluster] = LAST_CLUSTER;
  }



  // Ensure that the last cluster in the chain is marked "LAST_CLUSTER".
  // The one exception to this is if we're writing 0 bytes in total, in which
  // case we don't want to use any clusters at all.
  if (fat32_fat_entry_type(fs->fat[cur_cluster]) != LAST_CLUSTER) {
    fs->fat[cur_cluster] = LAST_CLUSTER;
  }
  write_fat_to_disk(fs);
}

int fat32_rename(fat32_fs_t *fs, pi_dirent_t *directory, char *oldname, char *newname) {
  // Get the dirents `directory` off the disk, and iterate through them
  // looking for the file.  When you find it, rename it and write it back to
  // the disk (validate the name first).  Return 0 in case of an error, or 1
  // on success.
  // Consider:
  //  - what do you do when there's already a file with the new name?
  demand(init_p, "fat32 not initialized!");
  if (trace_p) trace("renaming %s to %s\n", oldname, newname);
  if (!fat32_is_valid_name(newname)) return 0;

  // Get the dirents and find the right one
  uint32_t dir_n;
  fat32_dirent_t *cur_dirents = get_dirents(fs, directory->cluster_id, &dir_n);
  // pi_dirent_t *cur_dirent = fat32_stat(fs, directory, oldname);


  // Update the dirent's name
  int n = find_dirent_with_name(cur_dirents, dir_n, oldname);
  if (find_dirent_with_name(cur_dirents, dir_n, newname) != -1) {
    if (trace_p) trace("there is already a directory with name %s\n", newname);
    return 0;
  }
  fat32_dirent_set_name(cur_dirents+n, newname);



  // Write out the directory, using the existing cluster chain (or
  // appending to the end); implementing `write_cluster_chain` will help
  // printk("%d\n", directory->cluster_id);
  write_cluster_chain(fs, directory->cluster_id, (uint8_t *) cur_dirents, sizeof(fat32_dirent_t) * dir_n);

  return 1;

}

// Create a new directory entry for an empty file (or directory).
pi_dirent_t *fat32_create(fat32_fs_t *fs, pi_dirent_t *directory, char *filename, int is_dir) {
  demand(init_p, "fat32 not initialized!");
  if (trace_p) trace("creating %s\n", filename);
  if (!fat32_is_valid_name(filename)) return NULL;

  // Read the dirents and make sure there isn't already a file with the
  // same name
  uint32_t dir_n;
  fat32_dirent_t *cur_dirents = get_dirents(fs, directory->cluster_id, &dir_n);
  if (find_dirent_with_name(cur_dirents, dir_n, filename) != -1) {
    if (trace_p) trace("there is already a directory with name %s\n", filename);
    return NULL;
  }

  // Look for a free directory entry and use it to store the data for the
  // new file.  If there aren't any free directory entries, either panic or
  // (better) handle it appropriately by extending the directory to a new
  // cluster.
  // When you find one, update it to match the name and attributes
  // specified; set the size and cluster to 0.
  int i = 0;
  int idx = -1;
  while (cur_dirents[i].filename[0] != 0) {
    if(cur_dirents[i].filename[0] == 0xE5) {
      memset(&cur_dirents[i], 0, sizeof(fat32_dirent_t));
      fat32_dirent_set_name(cur_dirents + i, filename);
      cur_dirents[i].attr = is_dir << 4;
      idx = i;
      break;
    }
    i++;
  }
  if (idx == -1) {
    //panic("did not find an empty dirent slot to put new entry");
    //printk("fuck\n");
    if (i == dir_n - 1) {
      panic("no more space for more files?!?!?!\n");
    }
    memset(&cur_dirents[i + 1], 0, sizeof(fat32_dirent_t));
    memset(&cur_dirents[i], 0, sizeof(fat32_dirent_t));
    fat32_dirent_set_name(cur_dirents + i, filename);
    cur_dirents[i].attr = is_dir << 4;
    idx = i;
  }

  // Write out the updated directory to the disk
  write_cluster_chain(fs, directory->cluster_id, (uint8_t *) cur_dirents, sizeof(fat32_dirent_t) * dir_n);

  // Convert the dirent to a `pi_dirent_t` and return a (kmalloc'ed)
  // pointer
  pi_dirent_t *dirent = kmalloc(sizeof(pi_dirent_t));
  dirent[0] = dirent_convert(&cur_dirents[idx]);
  return dirent;
}

// Delete a file, including its directory entry.
int fat32_delete(fat32_fs_t *fs, pi_dirent_t *directory, char *filename) {
  demand(init_p, "fat32 not initialized!");
  if (trace_p) trace("deleting %s\n", filename);
  if (!fat32_is_valid_name(filename)) return 0;
  // Look for a matching directory entry, and set the first byte of the
  // name to 0xE5 to mark it as free
  uint32_t dir_n;
  fat32_dirent_t *cur_dirents = get_dirents(fs, directory->cluster_id, &dir_n);
  int found_idx = find_dirent_with_name(cur_dirents, dir_n, filename);
  if (found_idx == -1) {
    if (trace_p) trace("there is no file named %s, nothing to delete.\n", filename);
    return 0;
  }
  cur_dirents[found_idx].filename[0] = 0xE5;

  // Free the clusters referenced by this dirent
  uint32_t cur_cluster = cur_dirents[found_idx].lo_start | (cur_dirents[found_idx].hi_start << 16);
  uint32_t prev_cluster;
  while (fat32_fat_entry_type(cur_cluster) != LAST_CLUSTER) {
    prev_cluster = cur_cluster;
    cur_cluster = fs->fat[cur_cluster];
    fs->fat[prev_cluster] = FREE_CLUSTER;
  }
  fs->fat[cur_cluster] = FREE_CLUSTER;
  write_fat_to_disk(fs);

  // Write out the updated directory to the disk
  write_cluster_chain(fs, directory->cluster_id, (uint8_t *) cur_dirents, sizeof(fat32_dirent_t) * dir_n);
  return 1;
}

int fat32_truncate(fat32_fs_t *fs, pi_dirent_t *directory, char *filename, unsigned length) {
  demand(init_p, "fat32 not initialized!");
  if (trace_p) trace("truncating %s\n", filename);

  if (length == 0) {
    return fat32_delete(fs, directory, filename);
  }

  // Edit the directory entry of the file to list its length as `length` bytes.
  // Modify the cluster chain to either free unused clusters or add new
  // clusters.
  // Consider: what if the file we're truncating has length 0? what if we're
  // truncating to length 0?
  uint32_t dir_n;
  fat32_dirent_t *cur_dirents = get_dirents(fs, directory->cluster_id, &dir_n);
  int found_idx = find_dirent_with_name(cur_dirents, dir_n, filename);
  if (found_idx == -1) {
    if (trace_p) trace("there is no file named %s, nothing to truncate.\n", filename);
    return 0;
  }
  if (cur_dirents[found_idx].file_nbytes == 0) {
    uint8_t *data = kmalloc(length);
    memset((void*)data, 0, length);
    uint32_t ne_free = find_free_cluster(fs, 3);
    cur_dirents[found_idx].lo_start = ne_free & 0xFFFF;
    cur_dirents[found_idx].hi_start = (ne_free >> 16) & 0xFFFF;
    write_cluster_chain(fs, cur_dirents[found_idx].lo_start | cur_dirents[found_idx].hi_start << 16, data, length);
  } else {
    pi_file_t *file = fat32_read(fs, directory, filename);
    uint8_t *data = kmalloc(length);
    memset((void*)data, 0, length);
    memcpy((void*)data, file->data, length < file->n_data ? length : file->n_data);
    write_cluster_chain(fs, cur_dirents[found_idx].lo_start | cur_dirents[found_idx].hi_start << 16, data, length);
  }
  cur_dirents[found_idx].file_nbytes = length; 


  // Write out the directory entry
  write_cluster_chain(fs, directory->cluster_id, (uint8_t *) cur_dirents, sizeof(fat32_dirent_t) * dir_n);
  return 1;
}

int fat32_write(fat32_fs_t *fs, pi_dirent_t *directory, char *filename, pi_file_t *file) {
  demand(init_p, "fat32 not initialized!");
  demand(directory->is_dir_p, "tried to use a file as a directory!");

  // Surprisingly, this one should be rather straightforward now.
  // - load the directory
  // - exit with an error (0) if there's no matching directory entry
  // - update the directory entry with the new size
  // - write out the file as clusters & update the FAT
  // - write out the directory entry
  // Special case: the file is empty to start with, so we need to update the
  // start cluster in the dirent
  //printk("hi1\n");
  uint32_t dir_n;
  fat32_dirent_t *cur_dirents = get_dirents(fs, directory->cluster_id, &dir_n);
  int found_idx = find_dirent_with_name(cur_dirents, dir_n, filename);
  if (found_idx == -1) {
    if (trace_p) trace("there is no file named %s, nothing to write to.\n", filename);
    return 0;
  }
  //printk("hi2\n");
  if (cur_dirents[found_idx].file_nbytes == 0) {
    //printk("hi3\n");
    uint32_t ne_free = find_free_cluster(fs, 3);
    cur_dirents[found_idx].lo_start = ne_free & 0xFFFF;
    cur_dirents[found_idx].hi_start = (ne_free >> 16) & 0xFFFF;
    write_cluster_chain(fs, ne_free, file->data, file->n_data);
  } else {
    //printk("hi4\n");
    write_cluster_chain(fs, cur_dirents[found_idx].lo_start | cur_dirents[found_idx].hi_start << 16, file->data, file->n_data);
  }
  //printk("hi5\n");

  cur_dirents[found_idx].file_nbytes = file->n_data;


  write_cluster_chain(fs, directory->cluster_id, (uint8_t *) cur_dirents, sizeof(fat32_dirent_t) * dir_n);
  return 1;
}

int fat32_flush(fat32_fs_t *fs) {
  demand(init_p, "fat32 not initialized!");
  // no-op
  return 0;
}
