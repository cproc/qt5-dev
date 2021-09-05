// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/genode/video_capture_device_genode.h"

#include <stddef.h>
#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "media/capture/mojom/image_capture_types.h"
#include "media/capture/video/blob_utils.h"
#include "media/capture/video_capture_types.h"

namespace media {


// static
bool VideoCaptureDeviceGenode::GetVideoCaptureFormat(
    VideoCaptureFormat* video_format) {

  VideoCaptureFormat format;
  format.pixel_format = PIXEL_FORMAT_ARGB;
  format.frame_size.set_width(1280);
  format.frame_size.set_height(720);
  format.frame_rate = 30;
  if (!format.IsValid())
    return false;

  *video_format = format;

  return true;
}


VideoCaptureDeviceGenode::VideoCaptureDeviceGenode()
: capture_thread_("CaptureThread") {}


VideoCaptureDeviceGenode::~VideoCaptureDeviceGenode() {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Check if the thread is running.
  // This means that the device have not been DeAllocated properly.
  CHECK(!capture_thread_.IsRunning());
}

void VideoCaptureDeviceGenode::AllocateAndStart(
    const VideoCaptureParams& params,
    std::unique_ptr<VideoCaptureDevice::Client> client) {
fprintf(stderr, "*** VideoCaptureDeviceGenode::AllocateAndStart()\n");
  DCHECK(thread_checker_.CalledOnValidThread());
  CHECK(!capture_thread_.IsRunning());

  capture_thread_.Start();
  capture_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&VideoCaptureDeviceGenode::OnAllocateAndStart,
                 base::Unretained(this), params, base::Passed(&client)));
}

void VideoCaptureDeviceGenode::StopAndDeAllocate() {
fprintf(stderr, "*** VideoCaptureDeviceGenode::StopAndDeAllocate()\n");
  DCHECK(thread_checker_.CalledOnValidThread());
  CHECK(capture_thread_.IsRunning());

  capture_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&VideoCaptureDeviceGenode::OnStopAndDeAllocate,
                                base::Unretained(this)));
  capture_thread_.Stop();
}

void VideoCaptureDeviceGenode::GetPhotoState(GetPhotoStateCallback callback) {
fprintf(stderr, "*** VideoCaptureDeviceGenode::GetPhotoStateCallback()\n");

  DCHECK(thread_checker_.CalledOnValidThread());

  auto photo_capabilities = mojo::CreateEmptyPhotoState();

  int height = capture_format_.frame_size.height();
  photo_capabilities->height = mojom::Range::New(height, height, height, 0);
  int width = capture_format_.frame_size.width();
  photo_capabilities->width = mojom::Range::New(width, width, width, 0);

  std::move(callback).Run(std::move(photo_capabilities));
}

void VideoCaptureDeviceGenode::SetPhotoOptions(mojom::PhotoSettingsPtr settings,
                                             SetPhotoOptionsCallback callback) {
fprintf(stderr, "*** VideoCaptureDeviceGenode::SetPhotoOptions()\n");

  DCHECK(thread_checker_.CalledOnValidThread());

  if (settings->has_height &&
      settings->height != capture_format_.frame_size.height()) {
    return;
  }

  if (settings->has_width &&
      settings->width != capture_format_.frame_size.width()) {
    return;
  }

  if (settings->has_torch && settings->torch)
    return;

  if (settings->has_red_eye_reduction && settings->red_eye_reduction)
    return;

  if (settings->has_exposure_compensation || settings->has_exposure_time ||
      settings->has_color_temperature || settings->has_iso ||
      settings->has_brightness || settings->has_contrast ||
      settings->has_saturation || settings->has_sharpness ||
      settings->has_focus_distance || settings->has_zoom ||
      settings->has_fill_light_mode) {
    return;
  }

  std::move(callback).Run(true);
}

void VideoCaptureDeviceGenode::TakePhoto(TakePhotoCallback callback) {
fprintf(stderr, "*** VideoCaptureDeviceGenode::TakePhoto()\n");

  DCHECK(thread_checker_.CalledOnValidThread());
  base::AutoLock lock(lock_);

  take_photo_callbacks_.push(std::move(callback));
}

void VideoCaptureDeviceGenode::OnAllocateAndStart(
    const VideoCaptureParams& params,
    std::unique_ptr<VideoCaptureDevice::Client> client) {
fprintf(stderr, "*** VideoCaptureDeviceGenode::OnAllocateAndStart()\n");

  DCHECK(capture_thread_.task_runner()->BelongsToCurrentThread());

  client_ = std::move(client);

  GetVideoCaptureFormat(&capture_format_);

  client_->OnStarted();

  capture_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&VideoCaptureDeviceGenode::OnCaptureTask,
                                base::Unretained(this)));
}


void VideoCaptureDeviceGenode::OnStopAndDeAllocate() {
fprintf(stderr, "*** VideoCaptureDeviceGenode::OnStopAndDeAllocate()\n");
  DCHECK(capture_thread_.task_runner()->BelongsToCurrentThread());
  client_.reset();
  next_frame_time_ = base::TimeTicks();
}


void VideoCaptureDeviceGenode::OnCaptureTask() {
//fprintf(stderr, "*** VideoCaptureDeviceGenode::OnCaptureTask()\n");
  DCHECK(capture_thread_.task_runner()->BelongsToCurrentThread());
  if (!client_)
    return;
  base::AutoLock lock(lock_);

  // Give the captured frame to the client.

  static uint32_t buf[1280*720];
  static uint32_t val = 0xff000000;
  for (int i = 0; i < sizeof(buf)/sizeof(uint32_t); i++) {
  	buf[i] = val;
  	val++;
  }

  int frame_size = sizeof(buf);
  const uint8_t* frame_ptr = (const uint8_t*)buf;
  
  const base::TimeTicks current_time = base::TimeTicks::Now();
  if (first_ref_time_.is_null())
    first_ref_time_ = current_time;
  client_->OnIncomingCapturedData(frame_ptr, frame_size, capture_format_, 0,
                                  current_time, current_time - first_ref_time_);

  // Process waiting photo callbacks
  while (!take_photo_callbacks_.empty()) {
    auto cb = std::move(take_photo_callbacks_.front());
    take_photo_callbacks_.pop();

    mojom::BlobPtr blob =
        RotateAndBlobify(frame_ptr, frame_size, capture_format_, 0);
    if (!blob)
      continue;

    std::move(cb).Run(std::move(blob));
  }

  // Reschedule next CaptureTask.
  const base::TimeDelta frame_interval =
      base::TimeDelta::FromMicroseconds(1E6 / capture_format_.frame_rate);
  if (next_frame_time_.is_null()) {
    next_frame_time_ = current_time + frame_interval;
  } else {
    next_frame_time_ += frame_interval;
    // Don't accumulate any debt if we are lagging behind - just post next frame
    // immediately and continue as normal.
    if (next_frame_time_ < current_time)
      next_frame_time_ = current_time;
  }
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&VideoCaptureDeviceGenode::OnCaptureTask,
                     base::Unretained(this)),
      next_frame_time_ - current_time);
}

}  // namespace media
