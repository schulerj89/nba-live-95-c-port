#include "nba_setup_scheduler.h"
#include <stdio.h>
#include <string.h>

static void emit(void *context, const NbaSetupPublication *p) {
    (void)context;
    printf("op %u %u %u %u %u %u\n", p->mode, p->bbus, p->vmain,
           (unsigned)p->source, p->size, p->destination);
}

/* Probe input is an evidence protocol: reject overflow, signs, suffixes and
 * partial hex tokens before invoking the source primitive. scanf's unsigned
 * wrapping and prefix conversion are not suitable for this boundary. */
static int read_line(char *line, size_t capacity) {
    size_t length = 0;
    int c;
    while ((c = fgetc(stdin)) != EOF && c != '\n') {
        if (c == 0 || length + 1 >= capacity) return -1;
        line[length++] = (char)c;
    }
    line[length] = 0;
    return ferror(stdin) ? -1 : (c == EOF && length == 0 ? 0 : 1);
}

static int decimal(const char *token, uint32_t limit, uint32_t *result) {
    uint32_t value = 0;
    if (!*token) return 0;
    for (; *token; ++token) {
        uint32_t digit;
        if (*token < '0' || *token > '9') return 0;
        digit = (uint32_t)(*token - '0');
        if (value > limit / 10u || (value == limit / 10u && digit > limit % 10u)) return 0;
        value = value * 10u + digit;
    }
    *result = value;
    return 1;
}

static int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

int main(void) {
    char line[2048];
    NbaSetupEpoch epoch = {0};
    NbaSetupEpochWait wait = {0};
    int status;
    while ((status = read_line(line, sizeof(line))) > 0) {
        char *tokens[8];
        char *cursor = line;
        size_t count = 0;
        while (*cursor) {
            while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r') ++cursor;
            if (!*cursor) break;
            if (count == sizeof(tokens) / sizeof(tokens[0])) return 2;
            tokens[count++] = cursor;
            while (*cursor && *cursor != ' ' && *cursor != '\t' && *cursor != '\r') ++cursor;
            if (*cursor) *cursor++ = 0;
        }
        if (!count) return 2;
        const char *command = tokens[0];
        uint32_t a, b, c, d, e, f;
        if (strcmp(command, "queue") == 0) {
            NbaSetupPublicationQueue queue = {{0}, 0, 0, 0, 0, 0, 0};
            if (count != 8 || !decimal(tokens[1], 65535, &a) ||
                !decimal(tokens[2], 65535, &b) || !decimal(tokens[3], 65535, &c) ||
                !decimal(tokens[4], 65535, &d) || !decimal(tokens[5], 65535, &e) ||
                !decimal(tokens[6], 0xFFFFFF, &f) || strlen(tokens[7]) != 1024)
                return 2;
            for (size_t i = 0; i < sizeof(queue.records); ++i) {
                int high = hex_digit(tokens[7][i * 2]);
                int low = hex_digit(tokens[7][i * 2 + 1]);
                if (high < 0 || low < 0) return 2;
                queue.records[i] = (uint8_t)((high << 4) | low);
            }
            queue.head = (uint16_t)a;
            queue.tail = (uint16_t)b;
            queue.budget = (uint16_t)c;
            queue.palette_size = (uint16_t)d;
            queue.palette_destination = (uint16_t)e;
            queue.palette_source = f;
            NbaSetupQueueResult result = nba_setup_queue_publish(&queue, emit, NULL);
            printf("end %u %u %u %u\n", (unsigned)result, queue.head,
                   queue.budget, queue.palette_size);
        } else if (strcmp(command, "load") == 0 || strcmp(command, "load8") == 0) {
            if (count != 2 || !decimal(tokens[1], 65535, &a)) return 2;
            epoch.epoch = (uint16_t)a;
            nba_setup_epoch_wait_begin(&wait, &epoch, strcmp(command, "load8") == 0);
        } else if (strcmp(command, "state") == 0) {
            if (count != 4 || !decimal(tokens[1], 65535, &a) ||
                !decimal(tokens[2], 65535, &b) || !decimal(tokens[3], 1, &c)) return 2;
            epoch.epoch = (uint16_t)a;
            epoch.epoch_block = (uint16_t)b;
            epoch.interrupt_active = c != 0;
            printf("ready %u\n", nba_setup_epoch_wait_ready(&wait, &epoch));
        } else if (strcmp(command, "increment") == 0) {
            if (count != 1) return 2;
            nba_setup_epoch_nmi_increment(&epoch);
            printf("epoch %u\n", epoch.epoch);
        } else return 2;
    }
    return status < 0 ? 2 : 0;
}
