/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLISTS_BROWSER_PLAYLISTS_DOWNLOAD_CONTROLLER_H_
#define BRAVE_COMPONENTS_PLAYLISTS_BROWSER_PLAYLISTS_DOWNLOAD_CONTROLLER_H_

#include "base/macros.h"

class PlaylistsDownloadController {
 public:
  PlaylistsDownloadController();
  virtual ~PlaylistsDownloadController();

 private:
  DISALLOW_COPY_AND_ASSIGN(PlaylistsDownloadController);
};


#endif  // BRAVE_COMPONENTS_PLAYLISTS_BROWSER_PLAYLISTS_DOWNLOAD_CONTROLLER_H_
