static int solo_enc_fillbuf(struct solo_enc_dev *solo_enc,
			    struct vb2_buffer *vb, struct solo_enc_buf *enc_buf)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	const vop_header *vh = enc_buf->vh;
	int ret;

	switch (solo_enc->fmt) {
	case V4L2_PIX_FMT_MPEG4:
	case V4L2_PIX_FMT_H264:
		ret = solo_fill_mpeg(solo_enc, vb, vh);
		break;
	default: /* V4L2_PIX_FMT_MJPEG */
		ret = solo_fill_jpeg(solo_enc, vb, vh);
		break;
	}

	if (!ret) {
		vbuf->sequence = solo_enc->sequence++;
		v4l2_get_timestamp(&vbuf->timestamp);

		/* Check for motion flags */
		if (solo_is_motion_on(solo_enc) && enc_buf->motion) {
			struct v4l2_event ev = {
				.type = V4L2_EVENT_MOTION_DET,
				.u.motion_det = {
					.flags
					= V4L2_EVENT_MD_FL_HAVE_FRAME_SEQ,
					.frame_sequence = vbuf->sequence,
					.region_mask = enc_buf->motion ? 1 : 0,
				},
			};

			v4l2_event_queue(solo_enc->vfd, &ev);
		}
	}

	vb2_buffer_done(vb, ret ? VB2_BUF_STATE_ERROR : VB2_BUF_STATE_DONE);

	return ret;
}
