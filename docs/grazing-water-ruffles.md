# Swimming-height grazing reflection ruffles

The 2026-10-15 six-view visual critique found that the existing shallow crossing-wave colour was
readable from above but nearly disappeared at swimming height. This was physically consistent with
the high Fresnel reflection at a grazing angle, yet it left the near surface looking like one flat
cyan band.

`OceanRenderer` now reuses the same two analytic, derivative-filtered crossing phases to produce a
restrained signed reflection ruffle. It grows toward a grazing view, fades with the existing
135--420 m detail range, follows wind exposure, and borrows a small amount of the existing sky
reflection only at closed positive ripple cells. The subsurface term remains depth/Fresnel limited.
No new temporal frequency, noise lookup, texture, varying, mesh, buffer or draw was added.

Accepted native llvmpipe evidence is
`build-consumer/software-captures/20261015T034258.417110Z-water` (385 colour bins, 31.69 luminance
deviation, 131.68 spatial contrast; inspected). The preceding audit baseline is
`build-consumer/software-captures/20261015T031236.027478Z-water`. The final result remains a calm
lagoon surface but retains more local reflected structure in the low foreground.

Chrome/SwiftShader evidence
`build/browser-software/20261015T034445.077463Z-chrome-coast` passes the shipwreck, water, west-bay
and reef views for 120 finite positive intervals each, GL0, live contexts, distinct separated-time
frames and clean kernel monitoring; the water frame was inspected. ARM64 assemble/lint passes
without emulator/KVM (24,168,283-byte APK, SHA-256
`ec2529261ff14dad464ea5aca75ce3181179b86f6a836ed5e7232b6b7aca9024`, 19,125,712-byte stripped
`libmain.so`). Physical-mobile shimmer and performance remain unqualified.

## Aerial detail suppression

The later whole-island overview exposed the inverse problem: the same closed ripple cells that help
at swimming height remained fully visible from high above and formed a conspicuous regular
turquoise checker across the foreground shallows. The portable fragment shader now multiplies only
the two close cell terms by an altitude response that stays one through 28 m and fades to zero by
85 m. Wave displacement, slope normals, broad crossing-wave tint, depth/Fresnel response, foam and
wind exposure are unchanged. The solution adds no texture/noise sample, varying, mesh or draw.

The rejected baseline is `20261015T113456.468508Z-overview`. Accepted inspected native evidence is
`build-consumer/software-captures/20261015T113638.810594Z-overview`; independent low-water evidence
`build-consumer/software-captures/20261015T113718.051147Z-water` proves the original grazing detail
remains. Chrome/SwiftShader overview evidence
`build/browser-software/20261015T113837.885553Z-chrome-sky` passes 60 finite positive intervals,
GL0, live context and clean kernel monitoring. ARM64 assemble/lint passes without emulator/KVM;
the APK is 24,531,603 bytes with SHA-256
`db44063628cbde50aa068af0735d46e675be2c55f87fc64f80e0897cd9f1df3d` and contains a
19,489,032-byte `libmain.so`.
