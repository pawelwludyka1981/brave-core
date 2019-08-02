/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLISTS_BROWSER_PLAYLISTS_DB_CONTROLLER_H_
#define BRAVE_COMPONENTS_PLAYLISTS_BROWSER_PLAYLISTS_DB_CONTROLLER_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"

namespace leveldb {
class DB;
}  // namespace leveldb

struct PlaylistInfo;

class PlaylistsDBController {
 public:
  explicit PlaylistsDBController(const base::FilePath& db_path);
  virtual ~PlaylistsDBController();

  bool Init();
  void CreatePlaylist(const PlaylistInfo& playlist);

 private:
  base::FilePath db_path_;
  std::unique_ptr<leveldb::DB> db_;

  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(PlaylistsDBController);
};


#endif  // BRAVE_COMPONENTS_PLAYLISTS_BROWSER_PLAYLISTS_DB_CONTROLLER_H_
