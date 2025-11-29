package com.fitocube.backend.model.session;

import java.io.Serial;
import java.io.Serializable;

public record SessionUser(Long id, String userName, String displayName) implements Serializable {

    @Serial
    private static final long serialVersionUID = 1L;
}

