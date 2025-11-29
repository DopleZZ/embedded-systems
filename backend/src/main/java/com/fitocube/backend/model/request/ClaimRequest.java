package com.fitocube.backend.model.request;

import lombok.Data;

@Data
public class ClaimRequest {

    private String deviceUid;

    private String nickname;
}
