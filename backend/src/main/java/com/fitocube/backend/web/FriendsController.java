package com.fitocube.backend.web;

import com.fitocube.backend.model.FriendRequestDto;
import com.fitocube.backend.model.UserDto;
import com.fitocube.backend.model.request.CreateFriendRequest;
import com.fitocube.backend.model.session.SessionUser;
import com.fitocube.backend.services.FriendService;
import com.fitocube.backend.services.SessionService;
import java.util.List;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.lang.NonNull;
import org.springframework.security.core.annotation.AuthenticationPrincipal;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/friends")
public class FriendsController {

    private final FriendService friendService;
    private final SessionService sessionService;

    public FriendsController(FriendService friendService, SessionService sessionService) {
        this.friendService = friendService;
        this.sessionService = sessionService;
    }

    @GetMapping
    public ResponseEntity<List<UserDto>> listFriends(@RequestParam("userId") @NonNull Long userId) {
        var friends = friendService.listFriends(userId);
        return ResponseEntity.ok(friends);
    }

    @PostMapping("/request")
    public ResponseEntity<FriendRequestDto> sendFriendRequest(@RequestBody CreateFriendRequest request,
                                                              @AuthenticationPrincipal SessionUser user) {
        var current = sessionService.requireSessionUserEntity();
        var created = friendService.sendRequest(current, request);
        return ResponseEntity.status(HttpStatus.CREATED).body(created);
    }

    @GetMapping("/request/incoming")
    public ResponseEntity<List<FriendRequestDto>> listIncoming(@AuthenticationPrincipal SessionUser user) {
        var current = sessionService.requireSessionUserEntity();
        return ResponseEntity.ok(friendService.listIncoming(current));
    }

    @GetMapping("/request/outgoing")
    public ResponseEntity<List<FriendRequestDto>> listOutgoing(@AuthenticationPrincipal SessionUser user) {
        var current = sessionService.requireSessionUserEntity();
        return ResponseEntity.ok(friendService.listOutgoing(current));
    }

    @PostMapping("/request/{requestId}/accept")
    public ResponseEntity<UserDto> accept(@PathVariable("requestId") Long requestId,
                                          @AuthenticationPrincipal SessionUser user) {
        var current = sessionService.requireSessionUserEntity();
        var acceptedUser = friendService.acceptRequest(current, requestId);
        return ResponseEntity.ok(acceptedUser);
    }

    @PostMapping("/request/{requestId}/reject")
    public ResponseEntity<Void> reject(@PathVariable("requestId") Long requestId,
                                       @AuthenticationPrincipal SessionUser user) {
        var current = sessionService.requireSessionUserEntity();
        friendService.rejectRequest(current, requestId);
        return ResponseEntity.noContent().build();
    }
}
