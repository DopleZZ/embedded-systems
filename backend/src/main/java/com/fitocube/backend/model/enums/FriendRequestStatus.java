package com.fitocube.backend.model.enums;

import com.fasterxml.jackson.annotation.JsonValue;

public enum FriendRequestStatus {
    PENDING("pending"),
    ACCEPTED("accepted"),
    REJECTED("rejected");

    private final String value;

    FriendRequestStatus(String value) {
        this.value = value;
    }

    @JsonValue
    public String getValue() {
        return value;
    }
}
