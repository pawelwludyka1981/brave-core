/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLISTS_BROWSER_PLAYLISTS_CONTROLLER_H_
#define BRAVE_COMPONENTS_PLAYLISTS_BROWSER_PLAYLISTS_CONTROLLER_H_

#include <string>
#include <queue>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/playlists/browser/playlists_types.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
}  // namespace base

class PlaylistsControllerObserver;
class PlaylistsDBController;
class PlaylistsDownloadController;

class PlaylistsController {
 public:
  explicit PlaylistsController(
      scoped_refptr<base::SequencedTaskRunner> file_task_runner);
  virtual ~PlaylistsController();

  bool initialized() const { return initialized_; }

  void Init(const base::FilePath& base_dir);

  void CreatePlaylist(const CreatePlaylistParams& params);
  std::vector<PlaylistInfo> GetAllPlaylists();
  PlaylistInfo GetPlaylist(const std::string& id);
  void DeletePlaylist(const std::string& id);
  void DeleteAllPlaylists();

  void AddObserver(PlaylistsControllerObserver* observer);
  void RemoveObserver(PlaylistsControllerObserver* observer);

 private:
  void OnDBInitialized(bool initialized);

  bool initialized_ = false;
  std::queue<CreatePlaylistParams> pending_create_jobs_;
  base::ObserverList<PlaylistsControllerObserver> observers_;
  std::unique_ptr<PlaylistsDBController> db_;
  std::unique_ptr<PlaylistsDownloadController> download_manager_;
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  base::WeakPtrFactory<PlaylistsController> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PlaylistsController);
};

#endif  // BRAVE_COMPONENTS_PLAYLISTS_BROWSER_PLAYLISTS_CONTROLLER_H_
