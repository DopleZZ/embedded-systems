package com.fitocube.backend.services;

import com.fitocube.backend.model.FriendRequestDto;
import com.fitocube.backend.model.UserDto;
import com.fitocube.backend.model.enums.FriendRequestStatus;
import com.fitocube.backend.model.request.CreateFriendRequest;
import com.fitocube.backend.repositories.FriendRequestRepository;
import com.fitocube.backend.repositories.UserRepository;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.util.StringUtils;
import org.springframework.web.server.ResponseStatusException;

@Service
public class FriendService {

    private final FriendRequestRepository friendRequestRepository;
    private final UserRepository userRepository;

    public FriendService(FriendRequestRepository friendRequestRepository, UserRepository userRepository) {
        this.friendRequestRepository = friendRequestRepository;
        this.userRepository = userRepository;
    }

    public List<UserDto> listFriends(Long userId) {
        var user = userRepository.findById(userId).orElseThrow(() -> new ResponseStatusException(HttpStatus.NOT_FOUND));
        var friendIds = user.getFriends();
        if (friendIds == null || friendIds.isEmpty()) return List.of();
        return userRepository.findAllById(friendIds);
    }

    public List<FriendRequestDto> listIncoming(UserDto current) {
        return friendRequestRepository.findByTargetAndStatusOrderByCreatedAtDesc(current, FriendRequestStatus.PENDING);
    }

    public List<FriendRequestDto> listOutgoing(UserDto current) {
        return friendRequestRepository.findByRequesterAndStatusOrderByCreatedAtDesc(current, FriendRequestStatus.PENDING);
    }

    @Transactional
    public FriendRequestDto sendRequest(UserDto current, CreateFriendRequest request) {
        var requesterId = request.getRequesterId();
        var targetName = request.getTargetName();
        if (requesterId == null || requesterId.longValue() <= 0L) {
            throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "requesterId is required");
        }
        if (!StringUtils.hasText(targetName)) {
            throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "targetName is required");
        }
        if (!current.getUserId().equals(requesterId)) {
            throw new ResponseStatusException(HttpStatus.FORBIDDEN, "requesterId mismatch with session");
        }

        var target = userRepository.findByUserNameIgnoreCase(targetName)
                .orElseThrow(() -> new ResponseStatusException(HttpStatus.NOT_FOUND, "target user not found"));

        if (current.getUserId().equals(target.getUserId())) {
            throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "cannot friend yourself");
        }

        var alreadyFriends = current.getFriends() != null && current.getFriends().contains(target.getUserId());
        if (alreadyFriends) {
            throw new ResponseStatusException(HttpStatus.CONFLICT, "already friends");
        }

        var existingPending = friendRequestRepository.findByRequesterAndTargetAndStatus(current, target, FriendRequestStatus.PENDING);
        if (existingPending.isPresent()) {
            return existingPending.get();
        }

        var reversePending = friendRequestRepository.findByRequesterAndTargetAndStatus(target, current, FriendRequestStatus.PENDING);
        if (reversePending.isPresent()) {
            var rp = reversePending.get();
            acceptRequest(current, rp.getRequestId());
            rp.setStatus(FriendRequestStatus.ACCEPTED);
            return rp;
        }

        var fr = new FriendRequestDto();
        fr.setRequester(current);
        fr.setTarget(target);
        fr.setStatus(FriendRequestStatus.PENDING);
        return friendRequestRepository.save(fr);
    }

    @Transactional
    public FriendRequestDto getOrNotFound(Long requestId) {
        return friendRequestRepository.findById(requestId)
                .orElseThrow(() -> new ResponseStatusException(HttpStatus.NOT_FOUND, "request not found"));
    }

    @Transactional
    public UserDto acceptRequest(UserDto current, Long requestId) {
        var fr = getOrNotFound(requestId);
        if (!fr.getTarget().getUserId().equals(current.getUserId())) {
            throw new ResponseStatusException(HttpStatus.FORBIDDEN, "cannot accept not-your request");
        }
        if (fr.getStatus() != FriendRequestStatus.PENDING) {
            throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "request not pending");
        }
        fr.setStatus(FriendRequestStatus.ACCEPTED);
        friendRequestRepository.save(fr);

        var requester = fr.getRequester();
        var target = fr.getTarget();

        addFriendship(requester, target);
        return requester;
    }

    @Transactional
    public void rejectRequest(UserDto current, Long requestId) {
        var fr = getOrNotFound(requestId);
        if (!fr.getTarget().getUserId().equals(current.getUserId())) {
            throw new ResponseStatusException(HttpStatus.FORBIDDEN, "cannot reject not-your request");
        }
        if (fr.getStatus() != FriendRequestStatus.PENDING) {
            return;
        }
        fr.setStatus(FriendRequestStatus.REJECTED);
        friendRequestRepository.save(fr);
    }

    @Transactional
    protected void addFriendship(UserDto a, UserDto b) {
        var aFriends = a.getFriends();
        var bFriends = b.getFriends();
        if (aFriends == null || bFriends == null) {
            throw new ResponseStatusException(HttpStatus.INTERNAL_SERVER_ERROR);
        }
        aFriends.add(b.getUserId());
        bFriends.add(a.getUserId());
        userRepository.saveAll(Set.of(a, b));
    }
}
