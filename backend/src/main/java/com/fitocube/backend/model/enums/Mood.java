package com.fitocube.backend.model.enums;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonValue;

public enum Mood {
    HAPPY,
    NORMAL,
    THIRSTY,
    DRY,
    HOT,
    COLD;

    @JsonCreator
    public static Mood fromJson(String value) {
        if (value == null) {
            return null;
        }
        try {
            return Mood.valueOf(value.trim().toUpperCase());
        }
        catch (IllegalArgumentException ex) {
            return null;
        }
    }

    @JsonValue
    public String toJson() {
        return name().toLowerCase();
    }
}
