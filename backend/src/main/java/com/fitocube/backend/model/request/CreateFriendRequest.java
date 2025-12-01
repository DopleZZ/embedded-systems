package com.fitocube.backend.model.request;

import lombok.Data;

@Data
public class CreateFriendRequest {
    private Long requesterId;
    private String targetName;
}
