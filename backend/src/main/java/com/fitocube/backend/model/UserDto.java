package com.fitocube.backend.model;

import java.util.List;
import lombok.Data;

@Data
public class UserDto {

    private Long userId;
    private String userName;
    private String displayName;
    private List<Long> friends;
}
