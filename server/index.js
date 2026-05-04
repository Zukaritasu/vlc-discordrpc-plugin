/*****************************************************************************
 * index.js: Discord Rich Presence plugin for VLC
 *****************************************************************************
 * Copyright (C) 2026 Zukaritasu
 *
 * Authors: Zukaritasu <zukaritasu@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *****************************************************************************/

import { Hono } from 'hono'

const MESSAGES_ERROR = {
    HASH_MISSING: "hash_missing",
    EMPTY_BODY: "empty_body",
    IMAGE_NOT_FOUND: "image_not_found",
    INVALID_ARGUMENTS: "invalid_arguments"
}

const MESSAGES_SUCCESS = {
    IMAGE_ALREADY_EXISTS: "image_already_exists",
    IMAGE_UPLOADED: "image_uploaded"
}

const app = new Hono()


app.get('/api/cover', async (c) => {
    const hash = c.req.query('hash');
    if (!hash) {
        return c.json({ status: "ok", message: MESSAGES_ERROR.HASH_MISSING }, 400);
    }

    const existingObject = await c.env.COVERS_BUCKET.head(hash);
    if (!existingObject) {
        return c.json({ status: "ok", message: MESSAGES_ERROR.IMAGE_NOT_FOUND }, 404);
    }

    const imageUrl = `https://covers.vlc-covers.workers.dev/api/static/${hash}`;

    return c.json({
        message: imageUrl,
        status: "ok"
    });
});

app.post('/api/cover/upload', async (c) => {
    const hash = c.req.query('hash');
    let extension = c.req.query('extension');

    if (!hash || !extension) {
        return c.json({ status: "error", message: MESSAGES_ERROR.INVALID_ARGUMENTS }, 400);
    }

    extension = extension.toLowerCase();

    const existingObject = await c.env.COVERS_BUCKET.head(hash);
    if (existingObject) {
        return c.json({ status: "ok", message: MESSAGES_SUCCESS.IMAGE_ALREADY_EXISTS });
    }

    const arrayBuffer = await c.req.arrayBuffer();
    if (!arrayBuffer || arrayBuffer.byteLength === 0) {
        return c.json({ status: "error", message: MESSAGES_ERROR.EMPTY_BODY }, 400);
    }

    await c.env.COVERS_BUCKET.put(hash, arrayBuffer, {
        httpMetadata: {
            contentType: `image/${extension}`,
        }
    });

    return c.json({ status: "ok", message: MESSAGES_SUCCESS.IMAGE_UPLOADED });
});

app.get('/api/static/:hash', async (c) => {
    const hash = c.req.param('hash');

    const object = await c.env.COVERS_BUCKET.get(hash);

    if (object === null) {
        return c.text(MESSAGES_ERROR.IMAGE_NOT_FOUND, 404);
    }

    const headers = new Headers();
    object.writeHttpMetadata(headers);
    headers.set('etag', object.httpEtag);
    headers.set('Cache-Control', 'public, max-age=31536000, immutable');

    return new Response(object.body, {
        headers,
    });
});

app.get('/', (c) => {
    return c.text('Hello!')
})

export default app
