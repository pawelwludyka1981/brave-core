/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlists/browser/playlists_db_controller.h"

#include "base/files/file_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/playlists/browser/playlists_types.h"
#include "third_party/leveldatabase/env_chromium.h"
#include "third_party/leveldatabase/src/include/leveldb/db.h"
#include "third_party/leveldatabase/src/include/leveldb/iterator.h"
#include "third_party/leveldatabase/src/include/leveldb/options.h"
#include "third_party/leveldatabase/src/include/leveldb/slice.h"
#include "third_party/leveldatabase/src/include/leveldb/status.h"

PlaylistsDBController::PlaylistsDBController(const base::FilePath& db_path)
    : db_path_(db_path) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

PlaylistsDBController::~PlaylistsDBController() {
}

bool PlaylistsDBController::Init() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!db_);

  leveldb_env::Options options;
  options.create_if_missing = true;
#if defined(OS_WIN)
  std::string path = base::UTF16ToUTF8(db_path_.value());
#else
  std::string path = db_path_.value();
#endif
  leveldb::Status status = leveldb_env::OpenDB(options, path, &db_);

  if (status.IsCorruption()) {
    LOG(ERROR) << "Deleting corrupt database";
    base::DeleteFile(db_path_, true);
    status = leveldb_env::OpenDB(options, path, &db_);
  }
  if (status.ok()) {
    CHECK(db_);
    return true;
  }
  LOG(WARNING) << "Unable to open " << path << ": "
               << status.ToString();
  return false;
}

void PlaylistsDBController::CreatePlaylist(const PlaylistInfo& playlist) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}
