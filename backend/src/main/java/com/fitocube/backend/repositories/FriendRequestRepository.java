package com.fitocube.backend.repositories;

import com.fitocube.backend.model.FriendRequestDto;
import com.fitocube.backend.model.UserDto;
import com.fitocube.backend.model.enums.FriendRequestStatus;
import java.util.List;
import java.util.Optional;
import org.springframework.data.jpa.repository.JpaRepository;

public interface FriendRequestRepository extends JpaRepository<FriendRequestDto, Long> {

    List<FriendRequestDto> findByTargetAndStatusOrderByCreatedAtDesc(UserDto target, FriendRequestStatus status);

    List<FriendRequestDto> findByRequesterAndStatusOrderByCreatedAtDesc(UserDto requester, FriendRequestStatus status);

    Optional<FriendRequestDto> findByRequesterAndTargetAndStatus(UserDto requester, UserDto target, FriendRequestStatus status);
}
