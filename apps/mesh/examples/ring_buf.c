#include "ring_buf.h"
#include <string.h>
#include <stdio.h>

struct ring_buf *ring_buf_init(void *buf, int total_size, void (*highwater_cb)(struct ring_buf *buf), void (*lowwater_cb)(struct ring_buf *buf))
{
	struct ring_buf *r = (struct ring_buf *)buf;
	r->total_size = total_size - sizeof(struct ring_buf) + 1;
	r->free_size_low = total_size / 2;
	r->free_size_high = (r->total_size * 3) / 4;
	r->highwater_cb = highwater_cb;
	r->lowwater_cb = lowwater_cb;
	ring_buf_reset(r);
	return r;
}

uint8_t ring_buf_is_empty(struct ring_buf *buf)
{
	return (buf->write_next == buf->read_next);
}

uint16_t ring_buf_num_items(struct ring_buf *buf)
{
	int free_size;

	free_size = buf->read_next - buf->write_next;

	if (free_size <= 0) free_size += buf->total_size;

	if (free_size > 0) free_size--; // free_size 最大值只有 total_size - 1

	return (buf->total_size - free_size - 1);
}

uint32_t ring_buf_free_items(struct ring_buf *buf)
{
	int free_size;

	free_size = buf->read_next - buf->write_next;

	if (free_size <= 0) free_size += buf->total_size;

	if (free_size > 0) free_size--;

	return free_size;
}

void ring_buf_reset(struct ring_buf *buf)
{
	buf->read_next = 0;
	buf->write_next = 0;
}

static int ring_buf_add_buffer(struct ring_buf *buf, const void *buffer, int size, int start)
{
	int remain = buf->total_size - start;

	if (remain >= size)
	{
		memcpy(buf->buffer + start, buffer, size);
		return start + size;
	}
	else
	{
		memcpy(buf->buffer + start, buffer, remain);
		size -= remain;
		memcpy(buf->buffer, (const uint8_t *)buffer + remain, size);
		return size;
	}
}

int ring_buf_peek_data(struct ring_buf *buf, f_ring_peek_data peek_data, void *extra)
{
	int r = 0;
	uint32_t read_next = buf->read_next;
	const uint32_t write_next = buf->write_next;
	// printf("read_next %d write_next %d\n", read_next, write_next);

	while (read_next != write_next)
	{
		int cnt = read_next > write_next ? buf->total_size - read_next : write_next - read_next;
		int has_more = read_next > write_next ? 1 : 0;
		int c = peek_data(buf->buffer + read_next, cnt, has_more, extra);

		if (c < 1)
			break;

		r += c;
		read_next += c;

		if (read_next >= buf->total_size) read_next = 0;
	}

	buf->read_next = read_next;

	int free_size = buf->read_next - buf->write_next;

	if (free_size <= 0) free_size += buf->total_size;

	if (free_size > 0) free_size--;

	if (free_size > buf->free_size_high)
	{
		if (1 == buf->watermark_event)
		{
			buf->lowwater_cb(buf);
		}

		buf->watermark_event = 0;
	}

	return r;
}

static void ring_buf_rpt_event(struct ring_buf *buf, int free_size)
{
	if ((free_size < buf->free_size_low) && (0 == buf->watermark_event))
	{
		buf->watermark_event = 1;
		buf->highwater_cb(buf);
		return;
	}

	if (free_size > buf->free_size_high)
	{
		buf->watermark_event = 0;
	}
}

int ring_buf_write_data(struct ring_buf *buf, const void *data, int len)
{
	uint16_t next;
	int free_size;

	next = buf->write_next;
	free_size = buf->read_next - buf->write_next;

	if (free_size <= 0) free_size += buf->total_size;

	if (free_size > 0) free_size--;

	if (len > free_size)
	{
		ring_buf_rpt_event(buf, free_size);
		return 0;
	}

	next = ring_buf_add_buffer(buf, data, len, next);
	buf->write_next = next < buf->total_size ? next : 0;

	ring_buf_rpt_event(buf, free_size - len);

	return len;
}
