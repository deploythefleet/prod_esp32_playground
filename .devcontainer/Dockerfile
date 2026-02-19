# Use the official Espressif IDF Docker image as base
# ALWAYS use a tagged version (starts with 'v') to 
# ensure build reproducibility. Release tags will change.
FROM espressif/idf:v5.5.2

ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

# Define global arguments
ARG DEBIAN_FRONTEND="noninteractive"
ARG USERNAME=ubuntu
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN apt update \
    && apt install -y socat sudo telnet mosquitto mosquitto-clients udev \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

# Default to the generic ESP32 target
ENV IDF_TARGET=esp32
COPY .bash_aliases /home/${USERNAME}/.bash_aliases

# Add IDF tools to bash session
RUN echo "source /opt/esp/idf/export.sh" >> /home/${USERNAME}/.bashrc
