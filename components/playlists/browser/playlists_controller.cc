/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlists/browser/playlists_controller.h"

#include "base/bind.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "brave/components/playlists/browser/playlists_controller_observer.h"
#include "brave/components/playlists/browser/playlists_db_controller.h"
#include "brave/components/playlists/browser/playlists_download_controller.h"

namespace {

const base::FilePath::StringType kDatabasePath(
    FILE_PATH_LITERAL("playlists_db"));

}  // namespace

PlaylistsController::PlaylistsController(
    scoped_refptr<base::SequencedTaskRunner> file_task_runner)
    : file_task_runner_(file_task_runner),
      weak_factory_(this) {
}

PlaylistsController::~PlaylistsController() {
  file_task_runner_->DeleteSoon(FROM_HERE, std::move(db_));
}

void PlaylistsController::Init(const base::FilePath& base_dir) {
  db_.reset(new PlaylistsDBController(base_dir.Append(kDatabasePath)));

  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(),
      FROM_HERE,
      base::BindOnce(&PlaylistsDBController::Init,
                     base::Unretained(db_.get())),
      base::BindOnce(&PlaylistsController::OnDBInitialized,
                     weak_factory_.GetWeakPtr()));
}

void PlaylistsController::OnDBInitialized(bool initialized) {
  initialized_ = initialized;

  if (!initialized_) {
    DVLOG(1) << __func__ << ": " << initialized_;
    return;
  }

  for (PlaylistsControllerObserver& obs : observers_)
    obs.OnPlaylistsInitialized();
}

void PlaylistsController::CreatePlaylist(const CreatePlaylistParams& params) {
  DCHECK(initialized_);
  pending_create_jobs_.push(params);

  if (pending_create_jobs_.size() == 1) {
    db_->CreatePlaylist(PlaylistInfo());
  } else {
    pending_create_jobs_.push(params);
  }
}

std::vector<PlaylistInfo> PlaylistsController::GetAllPlaylists() {
  DCHECK(initialized_);
  return std::vector<PlaylistInfo>();
}

PlaylistInfo PlaylistsController::GetPlaylist(const std::string& id) {
  DCHECK(initialized_);
  return PlaylistInfo();
}

void PlaylistsController::DeletePlaylist(const std::string& id) {
  DCHECK(initialized_);
}

void PlaylistsController::DeleteAllPlaylists() {
  DCHECK(initialized_);
}

void PlaylistsController::AddObserver(PlaylistsControllerObserver* observer) {
  observers_.AddObserver(observer);
}

void PlaylistsController::RemoveObserver(
    PlaylistsControllerObserver* observer) {
  observers_.RemoveObserver(observer);
}
