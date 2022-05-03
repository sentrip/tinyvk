//
// Created by jayjay on 2/5/22.
//

#include "catch.hpp"

#define TINYVK_IMPLEMENTATION
#include "tinyvk_renderpass.h"

static constexpr VkAttachmentDescription ATTACH_SWAPCHAIN {{},
    VK_FORMAT_B8G8R8A8_SNORM,
    VK_SAMPLE_COUNT_1_BIT,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
};

static constexpr VkAttachmentDescription ATTACH_COLOR {{},
    VK_FORMAT_R8G8B8A8_SNORM,
    VK_SAMPLE_COUNT_1_BIT,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
};

static constexpr VkAttachmentDescription ATTACH_DEPTH {{},
    VK_FORMAT_R32_SFLOAT,
    VK_SAMPLE_COUNT_1_BIT,
    VK_ATTACHMENT_LOAD_OP_LOAD,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
};



#define CHECK_SUBPASS(s, input_count, color_count, preserve_count, depth_index) \
    REQUIRE( input_count == s.inputAttachmentCount ); \
    REQUIRE( color_count == s.colorAttachmentCount ); \
    REQUIRE( preserve_count == s.preserveAttachmentCount ); \
    REQUIRE( depth_index == (depth_index == -1u ? -1u : s.pDepthStencilAttachment->attachment) )


TEST_CASE("renderpass::builder - single color/depth", "[tinyvk_test]")
{
    tinyvk::renderpass_desc::builder builder{};
    auto color = builder.attach(ATTACH_COLOR);
    auto depth = builder.attach(ATTACH_DEPTH);

    auto s0 = builder.subpass();
    s0.output(color);
    s0.output(depth);

    auto desc = builder.build();

    REQUIRE( 2 == desc.attach.size() );
    REQUIRE( 1 == desc.subpasses.size() );
    REQUIRE( desc.dependencies.empty() );
    CHECK_SUBPASS(desc.subpasses[0], 0, 1, 0, 1);
    REQUIRE( nullptr == desc.subpasses[0].pResolveAttachments );
}

// Adding dependencies for swapchain inputs

// Resolve attachment resolution

TEST_CASE("renderpass::builder - single color read after write", "[tinyvk_test]")
{
    tinyvk::renderpass_desc::builder builder{};
    auto color_in = builder.attach(ATTACH_COLOR);
    auto color_out = builder.attach(ATTACH_COLOR);

    auto s0 = builder.subpass();
    s0.output(color_in);

    auto s1 = builder.subpass();
    s1.input(color_in);
    s1.output(color_out);

    auto desc = builder.build();

    REQUIRE( 2 == desc.attach.size() );
    REQUIRE( 2 == desc.subpasses.size() );
    REQUIRE( 1 == desc.dependencies.size() );

    CHECK_SUBPASS(desc.subpasses[0], 0, 1, 0, -1u);
    REQUIRE( nullptr == desc.subpasses[0].pResolveAttachments );

    CHECK_SUBPASS(desc.subpasses[1], 1, 1, 0, -1u);
    REQUIRE( nullptr == desc.subpasses[1].pResolveAttachments );

    REQUIRE( desc.dependencies[0].srcSubpass == 0 );
    REQUIRE( desc.dependencies[0].dstSubpass == 1 );
}

TEST_CASE("renderpass::builder - single color write after read", "[tinyvk_test]")
{
    tinyvk::renderpass_desc::builder builder{};
    auto color = builder.attach(ATTACH_COLOR);

    auto s0 = builder.subpass();
    s0.input(color);

    auto s1 = builder.subpass();
    s1.output(color);

    auto desc = builder.build();

    REQUIRE( 1 == desc.attach.size() );
    REQUIRE( 2 == desc.subpasses.size() );
    REQUIRE( 1 == desc.dependencies.size() );

    CHECK_SUBPASS(desc.subpasses[0], 1, 0, 0, -1u);
    REQUIRE( nullptr == desc.subpasses[0].pResolveAttachments );

    CHECK_SUBPASS(desc.subpasses[1], 0, 1, 0, -1u);
    REQUIRE( nullptr == desc.subpasses[1].pResolveAttachments );

    REQUIRE( desc.dependencies[0].srcSubpass == 0 );
    REQUIRE( desc.dependencies[0].dstSubpass == 1 );
}

TEST_CASE("renderpass::builder - single color write after write", "[tinyvk_test]")
{
    tinyvk::renderpass_desc::builder builder{};
    auto color = builder.attach(ATTACH_COLOR);

    auto s0 = builder.subpass();
    s0.output(color);

    auto s1 = builder.subpass();
    s1.output(color, true);

    auto desc = builder.build();

    REQUIRE( 1 == desc.attach.size() );
    REQUIRE( 2 == desc.subpasses.size() );
    REQUIRE( 1 == desc.dependencies.size() );

    CHECK_SUBPASS(desc.subpasses[0], 0, 1, 0, -1u);
    REQUIRE( nullptr == desc.subpasses[0].pResolveAttachments );

    CHECK_SUBPASS(desc.subpasses[1], 0, 1, 0, -1u);
    REQUIRE( nullptr == desc.subpasses[1].pResolveAttachments );

    REQUIRE( desc.dependencies[0].srcSubpass == 0 );
    REQUIRE( desc.dependencies[0].dstSubpass == 1 );
}


// Adding dependencies for explicit subpass dependencies
//      explicit dependency between unrelated subpasses
//      explicit dependency that will affect implicit dependencies between subpasses

// Preserve attachment calculation for R-after-W
//      different preserves for dependency chain split

TEST_CASE("renderpass::builder - preserve attachments - preserved across single pass", "[tinyvk_test]")
{
    tinyvk::renderpass_desc::builder builder{};
    auto color = builder.attach(ATTACH_COLOR);
    auto pos = builder.attach(ATTACH_COLOR);
    auto norm = builder.attach(ATTACH_COLOR);
    auto final = builder.attach(ATTACH_COLOR);

    auto s0 = builder.subpass();
    s0.output(color);
    s0.output(pos);

    auto s1 = builder.subpass();
    s1.input(color);
    s1.output(norm);

    auto s2 = builder.subpass();
    s2.input(color);
    s2.input(pos);
    s2.input(norm);
    s2.output(final);

    auto desc = builder.build();

    REQUIRE( 4 == desc.attach.size() );
    REQUIRE( 3 == desc.subpasses.size() );
    REQUIRE( 3 == desc.dependencies.size() );

    CHECK_SUBPASS(desc.subpasses[0], 0, 2, 0, -1u);

    CHECK_SUBPASS(desc.subpasses[1], 1, 1, 1, -1u);
    REQUIRE( 1 == desc.subpasses[1].pPreserveAttachments[0] );

    CHECK_SUBPASS(desc.subpasses[2], 3, 1, 0, -1u);

    REQUIRE( desc.dependencies[0].srcSubpass == 0 );
    REQUIRE( desc.dependencies[0].dstSubpass == 1 );

    REQUIRE( desc.dependencies[1].srcSubpass == 0 );
    REQUIRE( desc.dependencies[1].dstSubpass == 2 );

    REQUIRE( desc.dependencies[2].srcSubpass == 1 );
    REQUIRE( desc.dependencies[2].dstSubpass == 2 );
}


TEST_CASE("renderpass::builder - preserve attachments - preserved across multiple passes", "[tinyvk_test]")
{
    tinyvk::renderpass_desc::builder builder{};
    auto color = builder.attach(ATTACH_COLOR);
    auto pos = builder.attach(ATTACH_COLOR);
    auto norm = builder.attach(ATTACH_COLOR);
    auto tex = builder.attach(ATTACH_COLOR);
    auto final = builder.attach(ATTACH_COLOR);

    auto s0 = builder.subpass();
    s0.output(color);
    s0.output(pos);

    auto s1 = builder.subpass();
    s1.input(color);
    s1.output(norm);

    auto s2 = builder.subpass();
    s2.depends_on(s1);
    s2.input(norm);
    s2.output(tex);

    auto s3 = builder.subpass();
    s3.input(color);
    s3.input(pos);
    s3.input(norm);
    s3.input(tex);
    s3.output(final);

    auto desc = builder.build();

    REQUIRE( 5 == desc.attach.size() );
    REQUIRE( 4 == desc.subpasses.size() );
    REQUIRE( 6 == desc.dependencies.size() );

    CHECK_SUBPASS(desc.subpasses[0], 0, 2, 0, -1u);

    CHECK_SUBPASS(desc.subpasses[1], 1, 1, 1, -1u);
    REQUIRE( 1 == desc.subpasses[1].pPreserveAttachments[0] );

    CHECK_SUBPASS(desc.subpasses[2], 1, 1, 2, -1u);
    REQUIRE( 0 == desc.subpasses[2].pPreserveAttachments[0] );
    REQUIRE( 1 == desc.subpasses[2].pPreserveAttachments[1] );

    CHECK_SUBPASS(desc.subpasses[3], 4, 1, 0, -1u);

    // TODO: Check dependencies
}
