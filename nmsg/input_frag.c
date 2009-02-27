/*
 * Copyright (c) 2009 by Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* Red-black nmsg_frag glue. */

static int
frag_cmp(struct nmsg_frag *e1, struct nmsg_frag *e2) {
	return (e2->id - e1->id);
}

RB_PROTOTYPE(frag_ent, nmsg_frag, link, frag_cmp);
RB_GENERATE(frag_ent, nmsg_frag, link, frag_cmp);

/* Convenience macros. */

#define FRAG_INSERT(buf, fent) do { \
	RB_INSERT(frag_ent, &((buf)->rbuf.nft.head), fent); \
} while(0)

#define FRAG_FIND(buf, fent, find) do { \
	fent = RB_FIND(frag_ent, &((buf)->rbuf.nft.head), find); \
} while(0)

#define FRAG_REMOVE(buf, fent) do { \
	RB_REMOVE(frag_ent, &((buf)->rbuf.nft.head), fent); \
} while(0)

#define FRAG_NEXT(buf, fent, fent_next) do { \
	fent_next = RB_NEXT(frag_ent, &((buf)->rbuf.nft.head), fent); \
} while(0)

/* Private. */

static nmsg_res
read_frag_buf(nmsg_buf buf, ssize_t msgsize, Nmsg__Nmsg **nmsg) {
	Nmsg__NmsgFragment *nfrag;
	nmsg_res res;
	struct nmsg_frag *fent, find;

	res = nmsg_res_again;

	nfrag = nmsg__nmsg_fragment__unpack(NULL, msgsize, buf->pos);

	/* find the fragment, else allocate a node and insert into the tree */
	find.id = nfrag->id;
	FRAG_FIND(buf, fent, &find);
	if (fent == NULL) {
		fent = malloc(sizeof(*fent));
		if (fent == NULL) {
			res = nmsg_res_memfail;
			goto read_frag_buf_out;
		}
		fent->id = nfrag->id;
		fent->last = nfrag->last;
		fent->rem = nfrag->last + 1;
		fent->ts = buf->rbuf.ts;
		fent->frags = calloc(1, sizeof(ProtobufCBinaryData) *
				     (fent->last + 1));
		if (fent->frags == NULL) {
			free(fent);
			res = nmsg_res_memfail;
			goto read_frag_buf_out;
		}
		FRAG_INSERT(buf, fent);
		buf->rbuf.nfrags += 1;
	}

	if (fent->frags[nfrag->current].data != NULL) {
		/* fragment has already been received, network problem? */
		goto read_frag_buf_out;
	}

	/* attach the fragment payload to the tree node */
	fent->frags[nfrag->current] = nfrag->fragment;

	/* decrement number of remaining fragments */
	fent->rem -= 1;

	/* detach the fragment payload from the NmsgFragment */
	nfrag->fragment.len = 0;
	nfrag->fragment.data = NULL;

	/* reassemble if all the fragments have been gathered */
	if (fent->rem == 0)
		res = reassemble_frags(buf, nmsg, fent);

read_frag_buf_out:
	nmsg__nmsg_fragment__free_unpacked(nfrag, NULL);
	return (res);
}

static nmsg_res
reassemble_frags(nmsg_buf buf, Nmsg__Nmsg **nmsg, struct nmsg_frag *fent) {
	nmsg_res res;
	size_t len, padded_len;
	uint8_t *payload, *ptr;
	unsigned i;

	res = nmsg_res_again;

	/* obtain total length of reassembled payload */
	len = 0;
	for (i = 0; i <= fent->last; i++) {
		assert(fent->frags[i].data != NULL);
		len += fent->frags[i].len;
	}

	/* round total length up to nearest kilobyte */
	padded_len = len;
	if (len % 1024 != 0)
		padded_len += 1024 - (len % 1024);

	ptr = payload = malloc(padded_len);
	if (payload == NULL) {
		return (nmsg_res_memfail);
	}

	/* copy into the payload buffer and deallocate frags */
	for (i = 0; i <= fent->last; i++) {
		memcpy(ptr, fent->frags[i].data, fent->frags[i].len);
		free(fent->frags[i].data);
		ptr += fent->frags[i].len;
	}
	free(fent->frags);

	/* decompress */
	if (buf->flags & NMSG_FLAG_ZLIB) {
		size_t ulen;
		u_char *ubuf, *zbuf;

		zbuf = (u_char *) payload;
		res = nmsg_zbuf_inflate(buf->zb, len, zbuf,
					&ulen, &ubuf);
		if (res != nmsg_res_success) {
			free(payload);
			goto reassemble_frags_out;
		}
		payload = ubuf;
		len = ulen;
		free(zbuf);
	}

	/* unpack the defragmented payload */
	*nmsg = nmsg__nmsg__unpack(NULL, len, payload);
	assert(*nmsg != NULL);
	free(payload);

	res = nmsg_res_success;

reassemble_frags_out:
	/* deallocate from tree */
	buf->rbuf.nfrags -= 1;
	FRAG_REMOVE(buf, fent);
	free(fent);

	return (res);
}

static void
free_frags(nmsg_buf buf) {
	struct nmsg_frag *fent, *fent_next;
	unsigned i;

	if (buf->type == nmsg_buf_type_read_file ||
	    buf->type == nmsg_buf_type_read_sock)
	{
		for (fent = RB_MIN(frag_ent, &buf->rbuf.nft.head);
		     fent != NULL;
		     fent = fent_next)
		{
			FRAG_NEXT(buf, fent, fent_next);
			for (i = 0; i <= fent->last; i++)
				free(fent->frags[i].data);
			free(fent->frags);
			FRAG_REMOVE(buf, fent);
			free(fent);
		}
	}
}

static void
gc_frags(nmsg_buf buf) {
	struct nmsg_frag *fent, *fent_next;
	unsigned i;

	for (fent = RB_MIN(frag_ent, &buf->rbuf.nft.head);
	     fent != NULL;
	     fent = fent_next)
	{
		FRAG_NEXT(buf, fent, fent_next);
		if (buf->rbuf.ts.tv_sec - fent->ts.tv_sec >=
		    NMSG_FRAG_GC_INTERVAL)
		{
			FRAG_NEXT(buf, fent, fent_next);
			for (i = 0; i <= fent->last; i++)
				free(fent->frags[i].data);
			free(fent->frags);
			FRAG_REMOVE(buf, fent);
			free(fent);
			buf->rbuf.nfrags -= 1;
		}
	}
}