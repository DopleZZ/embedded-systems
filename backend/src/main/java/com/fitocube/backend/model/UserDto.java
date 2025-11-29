package com.fitocube.backend.model;

import jakarta.persistence.*;

import java.util.LinkedHashSet;
import java.util.Set;
import lombok.Data;

@Data
@Entity
@Table(name = "users")
public class UserDto {

    @Id
    @GeneratedValue(strategy = GenerationType.SEQUENCE, generator = "user_seq")
    @SequenceGenerator(name = "user_seq", sequenceName = "user_seq", allocationSize = 1)
    private Long userId;

    @Column(name = "user_name", nullable = false, unique = true)
    private String userName;

    private String displayName;

    @ElementCollection
    @CollectionTable(name = "user_friends", joinColumns = @JoinColumn(name = "user_id"))
    @Column(name = "friend_id")
    private Set<Long> friends = new LinkedHashSet<>();

    @OneToMany(mappedBy = "owner")
    Set<PlantStateDto> plants;
}
