FROM gradle:8.7-jdk17-alpine AS builder

WORKDIR /workspace

COPY build.gradle settings.gradle gradlew ./
COPY gradle gradle
COPY src src

RUN chmod +x gradlew
RUN ./gradlew bootJar --no-daemon \
    && JAR_FILE=$(ls build/libs | grep -v plain | head -n 1) \
    && mv build/libs/$JAR_FILE app.jar

FROM eclipse-temurin:17-jre-alpine

WORKDIR /app

COPY --from=builder /workspace/app.jar app.jar

EXPOSE 8080

ENTRYPOINT ["java", "-jar", "app.jar"]
