#pragma once

char* encode64(uint64_t id, char* url);
uint64_t decode64(const char* url);
