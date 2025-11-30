package com.fitocube.backend.model.request;

import lombok.Data;

@Data
public class CreateUserRequest {

    private String userName;
    private String displayName;
    private String password;
}
